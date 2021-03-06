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

#ifdef SUPPORT_TRAIN
#include "src/train/model_impl.h"
#else
#include "src/model_impl.h"
#endif
#include "include/model.h"
#include "utils/log_adapter.h"

namespace mindspore::lite {

std::shared_ptr<Model> Model::Import(const char *model_buf, size_t size) {
  auto model = std::make_shared<Model>();
  model->modelImpl = ModelImpl::Import(model_buf, size);
  return model;
}

lite::Primitive *Model::GetOp(const std::string &name) const {
  MS_EXCEPTION_IF_NULL(modelImpl);
  return const_cast<Primitive *>(modelImpl->GetOp(name));
}

void Model::FreeMetaGraph() {
  MS_EXCEPTION_IF_NULL(modelImpl);
  return modelImpl->FreeMetaGraph();
}

const schema::MetaGraph *Model::GetMetaGraph() const {
  MS_EXCEPTION_IF_NULL(modelImpl);
  return modelImpl->GetMetaGraph();
}

std::shared_ptr<ModelImpl> Model::GetModelImpl() {
  MS_EXCEPTION_IF_NULL(modelImpl);
  return this->modelImpl;
}
}  // namespace mindspore::lite

