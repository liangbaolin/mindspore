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
#include <vector>
#include "src/runtime/kernel/arm/fp32/pad.h"
#include "src/runtime/kernel/arm/int8/pad_int8.h"
#include "schema/model_generated.h"
#include "src/kernel_factory.h"
#include "include/errorcode.h"
#include "include/context.h"

using mindspore::lite::KernelRegistrar;
using mindspore::lite::RET_ERROR;
using mindspore::lite::RET_OK;
using mindspore::schema::PrimitiveType_Pad;

namespace mindspore::kernel {

kernel::LiteKernel *CpuPadInt8KernelCreator(const std::vector<lite::tensor::Tensor *> &inputs,
                                            const std::vector<lite::tensor::Tensor *> &outputs,
                                            OpParameter *opParameter, const lite::Context *ctx,
                                            const kernel::KernelKey &desc) {
  auto *kernel = new (std::nothrow) PadInt8CPUKernel(opParameter, inputs, outputs, ctx);
  if (kernel == nullptr) {
    MS_LOG(ERROR) << "new PadCPUKernel failed.";
    return nullptr;
  }
  return kernel;
}

kernel::LiteKernel *CpuPadFp32KernelCreator(const std::vector<lite::tensor::Tensor *> &inputs,
                                            const std::vector<lite::tensor::Tensor *> &outputs,
                                            OpParameter *opParameter, const lite::Context *ctx,
                                            const kernel::KernelKey &desc) {
  auto *kernel = new (std::nothrow) PadCPUKernel(opParameter, inputs, outputs, ctx);
  if (kernel == nullptr) {
    MS_LOG(ERROR) << "new PadCPUKernel failed.";
    return nullptr;
  }
  return kernel;
}

kernel::LiteKernel *CpuPadKernelCreator(const std::vector<lite::tensor::Tensor *> &inputs,
                                        const std::vector<lite::tensor::Tensor *> &outputs, OpParameter *opParameter,
                                        const lite::Context *ctx, const kernel::KernelKey &desc) {
  MS_ASSERT(opParameter != nullptr);
  MS_ASSERT(desc.type == schema::PrimitiveType_Concat);
  auto input_tensor = inputs.at(kInputIndex);
  auto data_type = input_tensor->data_type();
  kernel::LiteKernel *kernel = nullptr;
  switch (data_type) {
    case kNumberTypeInt8:
      kernel = CpuPadInt8KernelCreator(inputs, outputs, opParameter, ctx, desc);
      break;
    case kNumberTypeFloat32:
      kernel = CpuPadFp32KernelCreator(inputs, outputs, opParameter, ctx, desc);
      break;
    default:
      break;
  }

  if (kernel == nullptr) {
    MS_LOG(ERROR) << "kernel is nullptr.";
    return nullptr;
  }

  auto ret = kernel->Init();
  if (ret != RET_OK) {
    delete kernel;
    MS_LOG(ERROR) << "Init kernel failed, name: " << opParameter->name_ << ", type: "
                  << schema::EnumNamePrimitiveType(static_cast<schema::PrimitiveType>(opParameter->type_));
    return nullptr;
  }
  return kernel;
}

REG_KERNEL(kCPU, PrimitiveType_Pad, CpuPadKernelCreator)
}  // namespace mindspore::kernel
