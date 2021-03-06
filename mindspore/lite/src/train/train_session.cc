/**
 * Copyright 2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <algorithm>
#include "mindspore/lite/src/train/train_session.h"
#include "mindspore/lite/src/kernel_factory.h"
#include "mindspore/lite/src/param_value_lite.h"
#include "common/utils.h"
#include "mindspore/lite/src/ops/ops.h"
#include "ir/anf.h"
#include "mindspore/lite/src/ir/tensor.h"
#include "abstract/abstract_value.h"
#include "backend/session/anf_runtime_algorithm.h"
#include "src/ir/primitive_value.h"

namespace mindspore {
namespace session {
static std::vector<int> GetAnfNodeOutDims(const AnfNodePtr &anfNodePtr) {
  auto nodeAbstract = anfNodePtr->abstract();
  if (nodeAbstract != nullptr) {
    auto shape = nodeAbstract->GetShapeTrack();
    if (!shape->isa<abstract::Shape>()) {
      MS_LOG(EXCEPTION) << "Not a Shape";
      return {};
    }
    auto dims = dyn_cast<abstract::Shape>(shape)->shape();
    return dims;
  } else {
    MS_LOG(WARNING) << "abstract is nullptr, return empty dims";
    return {};
  }
}

static schema::Format GetAnfNodeFormat(const AnfNodePtr &anfNodePtr) {
  auto nodeAbstract = anfNodePtr->abstract();
  if (nodeAbstract != nullptr) {
    return schema::Format_NHWC;  // XXX TODO -- extract Format from AnfNode
  } else {
    MS_LOG(WARNING) << "abstract is nullptr, return schema::Format_NHWC";
    return schema::Format_NHWC;
  }
}

static TypeId GetAnfNodeOutTypeId(const AnfNodePtr &anfNodePtr) {
  auto nodeAbstract = anfNodePtr->abstract();
  if (nodeAbstract != nullptr) {
    return nodeAbstract->GetTypeTrack()->type_id();
  } else {
    MS_LOG(WARNING) << "abstract is nullptr, return kTypeUnknown";
    return TypeId::kTypeUnknown;
  }
}

int TrainSession::BuildKernelInputAndOutputFromFuncGraph(const KernelGraphPtr &kernel_graph) {
  auto return_node = kernel_graph->get_return();
  auto node_list = TopoSort(return_node);
  for (auto &node : node_list) {
    if (!node->isa<CNode>()) {
      continue;
    }
    KernelRelation kernel_relation;
    auto cnode = node->cast<CNodePtr>();
    kernel_relation.node_full_name = cnode->fullname_with_scope();
    kernel_relation.cnode = cnode;
    auto *out_tensor =
      new tensor::Tensor(GetAnfNodeOutTypeId(cnode), GetAnfNodeOutDims(cnode), GetAnfNodeFormat(cnode),
                         schema::NodeType_Parameter);
    kernel_relation.output_tensor.push_back(out_tensor);
    tensor::Tensor *tensor_ptr = nullptr;
    for (size_t index = 1; index < cnode->inputs().size(); ++index) {
      if (cnode->input(index)->isa<CNode>()) {
        auto input_cnode = cnode->input(index)->cast<CNodePtr>();
        auto input_kernel_relation = kernel_relation_infos_[input_cnode->fullname_with_scope()];
        // todo not support multi-outputs kernel sudo as spilt
        tensor_ptr = input_kernel_relation.output_tensor.front();
      } else if (cnode->input(index)->isa<Parameter>()) {
        auto input_parameter = cnode->input(index)->cast<ParameterPtr>();
        auto para = input_parameter->default_param();
        auto param_value = std::dynamic_pointer_cast<ParamValueLite>(para);
        auto dims = param_value->tensor_shape();
        tensor_ptr = new tensor::Tensor(param_value->tensor_type(), dims, schema::Format_NHWC,
                                        schema::NodeType_ValueNode);  // XXX TODO -- extract Format from AnfNode
        if (param_value->tensor_size() != 0) {
          tensor_ptr->SetData(param_value->tensor_addr());
        }
      } else if (cnode->input(index)->isa<ValueNode>()) {
        auto input_valuenode = cnode->input(index)->cast<ValueNodePtr>();
        tensor_ptr = new tensor::Tensor(GetAnfNodeOutTypeId(input_valuenode), GetAnfNodeOutDims(input_valuenode),
                                        schema::Format_NHWC,
                                        schema::NodeType_Parameter);  // XXX TODO -- extract Format from AnfNode
        // todo(yankai)
      } else {
        MS_ASSERT(false);
      }
      kernel_relation.input_tensor.push_back(tensor_ptr);
    }
    kernel_relation_infos_[cnode->fullname_with_scope()] = kernel_relation;
  }
  return 0;
}

GraphId TrainSession::CompileGraph(const AnfNodePtrList &lst, const AnfNodePtrList &outputs) {
  auto graph_id = graph_sum_;
  auto graph = SessionBasic::ConstructKernelGraph(lst, outputs);
  MS_EXCEPTION_IF_NULL(graph);

  BuildKernel(graph.get());
  MS_LOG(INFO) << "Assign kernel address";
  runtime_.AssignKernelAddress(graph.get());
  return graph_id;
}

GraphId TrainSession::CompileGraph(const char *model_buf, size_t size) { return 0; }

std::shared_ptr<KernelGraph> TrainSession::ConstructKernelGraph(const FuncGraphPtr &func_graph) {
  MS_EXCEPTION_IF_NULL(func_graph);
  auto graph = NewKernelGraph();
  graph->set_return(func_graph->get_return());
  auto node_list = TopoSort(func_graph->get_return());
  std::vector<CNodePtr> cnode_order;
  for (const auto &node : node_list) {
    MS_EXCEPTION_IF_NULL(node);
    if (node->isa<CNode>()) {
      auto cn_node = node->cast<CNodePtr>();
      cnode_order.push_back(cn_node);
    }
  }
  graph->set_execution_order(cnode_order);
  return graph;
}

GraphId TrainSession::CompileGraph(NotNull<FuncGraphPtr> func_graph) {
  auto graph = ConstructKernelGraph(func_graph);
  MS_EXCEPTION_IF_NULL(graph);
  MS_LOG(INFO) << "Set kernel info";
  SetKernelInfo(graph.get());

  (void) BuildKernelInputAndOutputFromFuncGraph(graph);
  MS_LOG(INFO) << "Build kernel";
  auto ret = BuildKernel(graph.get());
  if (0 != ret) {
    MS_LOG(EXCEPTION) << "BuildKernel failed";
  }

  // return the graph id to backend
  auto graph_id = graph->graph_id();
  graphs_[graph_id] = graph;
  MS_LOG(INFO) << "Compile graph " << graph_id << " success";
  return graph_id;
}

void TrainSession::RunGraph(const GraphId &graph_id, const std::vector<tensor::Tensor *> &inputs,
                            std::vector<tensor::Tensor *> &outputs) {
  auto &kernel_graph = graphs_[graph_id];
  MS_EXCEPTION_IF_NULL(kernel_graph);
  MS_LOG(INFO) << "Bind input output address";
  runtime_.BindInputOutput(kernel_graph.get(), inputs, outputs);
  //  auto execution_order = kernel_graph->execution_order();
  // Todo : hangangqiang
  //  Reorder(&execution_order);
  //  kernel_graph->set_execution_order(execution_order);
  MS_LOG(INFO) << "Run graph start";
  auto ret = runtime_.Run(kernel_graph.get(), (std::vector<tensor::Tensor *> &) inputs, outputs);
  if (!ret) {
    MS_LOG(EXCEPTION) << "Run graph failed";
  }
  MS_LOG(INFO) << "Run graph end";
}

void TrainSession::SetKernelInfo(const KernelGraph *kernel_graph) {
  MS_EXCEPTION_IF_NULL(kernel_graph);
  auto &kernel_nodes = kernel_graph->execution_order();
  for (const auto &kernel_node : kernel_nodes) {
    MS_EXCEPTION_IF_NULL(kernel_node);
    auto kernel_info = std::make_shared<device::KernelInfo>();
    kernel_node->set_kernel_info(kernel_info);
  }
}

int TrainSession::BuildKernel(const KernelGraph *kernel_graph) {
  MS_EXCEPTION_IF_NULL(kernel_graph);
  for (auto iter = kernel_relation_infos_.begin(); iter != kernel_relation_infos_.end(); ++iter) {
    std::string kernel_name = iter->first;
    KernelRelation anf_register = iter->second;
    MS_EXCEPTION_IF_NULL(anf_register.cnode);
    if (IsPrimitiveCNode(anf_register.cnode, prim::kPrimReturn)) {
      continue;
    }
    lite::Context context;
    context.deviceCtx.type = lite::DeviceType::DT_CPU;
    auto value_node_prim = anf_register.cnode->input(0);
    MS_EXCEPTION_IF_NULL(value_node_prim);
    auto prim = GetValueNode<std::shared_ptr<lite::PrimitiveValue>>(value_node_prim);
    MS_EXCEPTION_IF_NULL(prim);
    auto node_primitive = (lite::Primitive *) (prim->GetPrimitive());
    MS_EXCEPTION_IF_NULL(node_primitive);
    auto ret = node_primitive->InferShape(anf_register.input_tensor, anf_register.output_tensor);
    if (0 != ret) {
      MS_LOG(ERROR) << "InferShape failed, node : " << kernel_name;
      return ret;
    }
    kernel::KernelKey desc{kernel::KERNEL_ARCH::kCPU, node_primitive->Type()};

    auto *kernel = lite::KernelFactory::GetInstance()->GetKernel(anf_register.input_tensor, anf_register.output_tensor,
                                                                 node_primitive, &context, desc);
    if (nullptr == kernel) {
      MS_LOG(ERROR) << "Create kernel return nullptr, name: " << kernel_name;
      return -1;
    }
    kernel->train();
    auto *kernel_info = anf_register.cnode->kernel_info();
    std::shared_ptr<kernel::LiteKernel> kernel_mod(kernel);
    kernel_info->set_kernel_mod(kernel_mod);
  }
  return 0;
}
}  // namespace session
}  // namespace mindspore

