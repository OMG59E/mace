//
// Copyright (c) 2017 XiaoMi All rights reserved.
//

#include "mace/kernels/conv_2d.h"

namespace mace {
namespace kernels {

extern void Conv2dOpenclK1x1S1(const Tensor *input, const Tensor *filter,
                               const Tensor *bias, const int dilation_height,
                               const int dilation_width, Tensor *output);

extern void Conv2dOpenclK3x3S1(const Tensor *input, const Tensor *filter,
                               const Tensor *bias, const int dilation_height,
                               const int dilation_width, Tensor *output);

extern void Conv2dOpenclK3x3S2(const Tensor *input, const Tensor *filter,
                               const Tensor *bias, const int dilation_height,
                               const int dilation_width, Tensor *output);
template <>
void Conv2dFunctor<DeviceType::OPENCL, float>::operator()(const Tensor *input,
                                                          const Tensor *filter,
                                                          const Tensor *bias,
                                                          Tensor *output) {
  typedef void (*Conv2dOpenclFunction)(const Tensor *input, const Tensor *filter,
                                       const Tensor *bias, const int dilation_height,
                                       const int dilation_width, Tensor *output);
  // Selection matrix: kernel_size x stride_size
  static const Conv2dOpenclFunction selector[5][2] = {
      {Conv2dOpenclK1x1S1, nullptr},
      {nullptr, nullptr},
      {Conv2dOpenclK3x3S1, Conv2dOpenclK3x3S2},
      {nullptr, nullptr},
      {nullptr, nullptr}};

  index_t kernel_h = filter->shape()[2];
  index_t kernel_w = filter->shape()[3];
  if (kernel_h != kernel_w || kernel_h > 5 || strides_[0] != strides_[1] ||
      strides_[0] > 2 || selector[kernel_h - 1][strides_[0] - 1] == nullptr) {
    LOG(WARNING) << "OpenCL conv2d kernel with "
                 << "filter" << kernel_h << "x" << kernel_w << ","
                 << " stride " << strides_[0] << "x" << strides_[1]
                 << " is not implemented yet, using slow version";
    // TODO(heliangliang) The CPU/NEON kernel should map the buffer
    Conv2dFunctor<DeviceType::CPU, float>(strides_, paddings_, dilations_)(
        input, filter, bias, output);
    return;
  }
  auto conv2d_func = selector[kernel_h - 1][strides_[0] - 1];
  if (paddings_[0] > 0 || paddings_[1] > 0) {
    Tensor padded_input(GetDeviceAllocator(DeviceType::OPENCL), DataTypeToEnum<float>::v());
    Tensor::MappingGuard input_mapper(input);
    ConstructInputWithPadding(input->data<float>(), input->shape().data(), paddings_.data(),
                              &padded_input);
    conv2d_func(&padded_input, filter, bias, dilations_[0], dilations_[1], output);
  }else {
    conv2d_func(input, filter, bias, dilations_[0], dilations_[1], output);
  }
}

}  // namespace kernels
}  // namespace mace
