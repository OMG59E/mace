// Copyright 2018 Xiaomi, Inc.  All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mace/kernels/fully_connected.h"
#include "mace/kernels/gemm.h"

namespace mace {
namespace kernels {

void FullyConnectedFunctor<DeviceType::NEON,
                           float>::operator()(const Tensor *input,
                                              const Tensor *weight,
                                              const Tensor *bias,
                                              Tensor *output,
                                              StatsFuture *future) {
  std::vector<index_t> output_shape = {input->dim(0), weight->dim(0), 1, 1};
  output->Resize(output_shape);
  const index_t N = output->dim(0);
  const index_t input_size = weight->dim(1);
  const index_t output_size = weight->dim(0);
  const float *input_ptr = input->data<float>();
  const float *weight_ptr = weight->data<float>();
  const float *bias_ptr = bias == nullptr ? nullptr : bias->data<float>();
  float *output_ptr = output->mutable_data<float>();

  for (int i = 0; i < N; ++i) {
    Gemv(weight_ptr, input_ptr, input_size, output_size, output_ptr);
    for (int j = 0; j < output_size; ++j) {
      output_ptr[j] += bias_ptr[j];
    }
  }

  DoActivation(output_ptr, output_ptr, output->size(), activation_,
               relux_max_limit_);
}

}  // namespace kernels
}  // namespace mace