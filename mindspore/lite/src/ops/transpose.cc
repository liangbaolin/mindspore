/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#include "src/ops/ops.h"
#include "include/errorcode.h"
#include "utils/log_adapter.h"
#include "src/ir/tensor.h"

namespace mindspore::lite {
int Transpose::InferShape(std::vector<tensor::Tensor *> inputs_, std::vector<tensor::Tensor *> outputs_) {
  MS_ASSERT(this->primitive != nullptr);
  auto input = inputs_.front();
  MS_ASSERT(input != nullptr);
  auto output = outputs_.front();
  MS_ASSERT(output != nullptr);

  MS_ASSERT(inputs_.size() == kSingleNum);
  MS_ASSERT(outputs_.size() == kSingleNum);
  auto transpore_prim = this->primitive->value_as_Transpose();
  int conjugate = transpore_prim->conjugate();
  if (conjugate) {
    MS_LOG(ERROR) << "Transpose conjugate is not support currently";
    return RET_ERROR;
  }
  std::vector<int> perm;
  perm.insert(perm.begin(), transpore_prim->perm()->begin(), transpore_prim->perm()->end());

  std::vector<int> in_shape = input->shape();
  std::vector<int> out_shape;
  out_shape.resize(perm.size());
  for (int i = 0; i < perm.size(); ++i) {
    out_shape[i] = in_shape[perm[i]];
  }

  output->set_shape(out_shape);
  output->set_data_type(input->data_type());
  output->SetFormat(input->GetFormat());

  return RET_OK;
}
}  // namespace mindspore::lite
