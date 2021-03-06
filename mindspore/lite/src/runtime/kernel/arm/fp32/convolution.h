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

#ifndef MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_FP32_CONVOLUTION_H_
#define MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_FP32_CONVOLUTION_H_

#include <vector>
#include "src/lite_kernel.h"
#include "src/runtime/kernel/arm/opclib/op_base.h"
#include "src/runtime/kernel/arm/base/convolution_base.h"

namespace mindspore::kernel {
class ConvolutionCPUKernel : public ConvolutionBaseCPUKernel {
 public:
  ConvolutionCPUKernel(OpParameter *parameter, const std::vector<lite::tensor::Tensor *> &inputs,
                       const std::vector<lite::tensor::Tensor *> &outputs, const Context *ctx)
      : ConvolutionBaseCPUKernel(parameter, inputs, outputs, ctx) {}
  ~ConvolutionCPUKernel() override {
    if (packed_input_ != nullptr) {
      free(packed_input_);
    }
    if (packed_weight_ != nullptr) {
      free(packed_weight_);
    }
    if (tmp_output_block_ != nullptr) {
      free(tmp_output_block_);
    }
  };

  int Init() override;
  int ReSize() override;
  int Run() override;
  int RunImpl(int task_id);
  int InitWeightBias();
  int InitTmpBuffer();
  void ConfigInputOutput();

 private:
  float *packed_input_;
  float *packed_weight_;
  float *tmp_output_block_;
};
}  // namespace mindspore::kernel

#endif  // MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_FP32_CONVOLUTION_H_

