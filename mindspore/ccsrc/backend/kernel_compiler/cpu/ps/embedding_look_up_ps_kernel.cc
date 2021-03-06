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

#include "backend/kernel_compiler/cpu/ps/embedding_look_up_ps_kernel.h"
#include <functional>
#include <vector>
#include <memory>
#include "backend/kernel_compiler/common_utils.h"
#include "frontend/parallel/ps/util.h"

namespace mindspore {
namespace kernel {
namespace ps {
using mindspore::parallel::ps::Util;
void EmbeddingLookUpPSKernel::InitKernel(
  const std::shared_ptr<std::vector<std::shared_ptr<std::vector<size_t>>>> &shapes) {
  const std::vector<std::shared_ptr<std::vector<size_t>>> &shape_vec = *shapes;
  input_shape_ = *(shape_vec[0]);
  input_lens_ = 1;
  for (auto shape : input_shape_) {
    input_lens_ = input_lens_ * shape;
  }
  indices_shape_ = *(shape_vec[1]);
  indices_lens_ = 1;
  for (auto shape : indices_shape_) {
    indices_lens_ = indices_lens_ * shape;
  }
  output_shape_ = *(shape_vec[2]);
  axis_ = 2;
  reduce_scatter_flag_ = false;

  size_t offset = 0;
  for (size_t i = 0; i < rank_id_; i++) {
    offset += Util::LocalShard(input_shape_[axis_], i, pserver_num_);
  }
  offset_ = offset;
  split_num_ = pserver_num_;

  // input shape should be sharded after computing offset_;
  Shard(&input_shape_, axis_);

  size_t output_size =
    std::accumulate(output_shape_.begin(), output_shape_.end(), sizeof(float), std::multiplies<size_t>());
  output_size_list_.emplace_back(output_size);
  CPUKernelUtils::ExpandDimsTo4(&input_shape_);
  CPUKernelUtils::ExpandDimsTo4(&output_shape_);
}

void EmbeddingLookUpPSKernel::ReInit(const std::shared_ptr<std::vector<std::shared_ptr<std::vector<size_t>>>> &shapes) {
  const std::vector<std::shared_ptr<std::vector<size_t>>> &shape_vec = *shapes;
  const auto &indices_shape_ = *(shape_vec[0]);
  indices_lens_ = indices_shape_[0];

  size_t output_size = sizeof(float) * indices_lens_;
  for (size_t i = axis_ + 1; i < input_shape_.size(); i++) {
    output_size *= input_shape_[i];
  }
  output_size_list_.clear();
  output_size_list_.emplace_back(output_size);
}

bool EmbeddingLookUpPSKernel::Execute(const std::vector<AddressPtr> &inputs, const std::vector<AddressPtr> &workspace,
                                      const std::vector<AddressPtr> &outputs) {
  return Launch(inputs, workspace, outputs);
}

const std::vector<size_t> &EmbeddingLookUpPSKernel::input_sizes() const { return input_shape_; }

const std::vector<size_t> &EmbeddingLookUpPSKernel::output_sizes() const { return GetOutputSizeList(); }

const std::vector<size_t> &EmbeddingLookUpPSKernel::workspace_sizes() const { return GetWorkspaceSizeList(); }
}  // namespace ps
}  // namespace kernel
}  // namespace mindspore
