/**
 * Copyright (c) 2016-present, Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "caffe2/core/context_hip.h"
#include "caffe2/core/miopen_wrapper.h"
#include "caffe2/operators/conv_pool_op_base.h"

namespace caffe2 {
class MIOPENPoolOp : public ConvPoolOpBase<HIPContext> {
 public:
  MIOPENPoolOp(const OperatorDef& operator_def, Workspace* ws)
      : ConvPoolOpBase<HIPContext>(operator_def, ws),
        miopen_wrapper_(&context_),
        alpha_(OperatorBase::GetSingleArgument<float>("alpha", 1.0)),
        beta_(OperatorBase::GetSingleArgument<float>("beta", 0.0)),
        do_backward_(OperatorBase::GetSingleArgument<bool>("do_backward", true)),
        poolWsSize_(0)

  {
    MIOPEN_ENFORCE(miopenCreateTensorDescriptor(&bottom_desc_));
    MIOPEN_ENFORCE(miopenCreateTensorDescriptor(&top_desc_));
    MIOPEN_ENFORCE(miopenCreatePoolingDescriptor(&pooling_desc_));

    if ((operator_def.type().substr(0, 9) == "MaxPool2D") || (operator_def.type().substr(0, 17) == "MaxPool2DGradient")){
      mode_ = miopenPoolingMax;
    }
    else if ((operator_def.type().substr(0, 13) == "AveragePool2D") || (operator_def.type().substr(0, 21) == "AveragePool2DGradient")) {
      mode_ =  miopenPoolingAverage;
    }
    else
    {
      LOG(FATAL) << "Unsupported pooling method: " << operator_def.type();
    }
  }

  ~MIOPENPoolOp() {
    MIOPEN_ENFORCE(miopenDestroyTensorDescriptor(bottom_desc_));
    MIOPEN_ENFORCE(miopenDestroyTensorDescriptor(top_desc_));
    MIOPEN_ENFORCE(miopenDestroyPoolingDescriptor(pooling_desc_));
    poolWsSize_ = 0;
  }

  template <typename T, typename M>
  bool DoRunWithType() {
    auto& X = Input(0);
    auto* Y = Output(0);
    int N = 0, C = 0, H = 0, W = 0, D = 0;
    int N_out = 0, C_out = 0, H_out = 0, W_out = 0;
    CAFFE_ENFORCE(X.ndim() >= 4 && X.ndim() <= 5);
    N = X.dim32(0);
    C = X.dim32(1);
    H = X.dim32(2);
    W = X.ndim() > 3 ? X.dim32(3) : 1;
    ConvPoolOpBase::SetOutputSize(X, Y, C);
    if (kernel_.size() == 2) {
        MIOPEN_ENFORCE(miopenSet2dPoolingDescriptor(
            pooling_desc_,
            mode_,
            kernel_h(),
            kernel_w(),
            pad_t(),
            pad_l(),
            stride_h(),
            stride_w()));
      }

    MIOPEN_ENFORCE(miopenSet4dTensorDescriptor(
            bottom_desc_,
            miopenTypeWrapper<T>::type,
            N,
            C,
            H,
            W));

    MIOPEN_ENFORCE(miopenGetPoolingForwardOutputDim(
            pooling_desc_,
            bottom_desc_,
            &N_out,
            &C_out,
            &H_out,
            &W_out));

    MIOPEN_ENFORCE(miopenSet4dTensorDescriptor(
            top_desc_,
            miopenTypeWrapper<T>::type,
            N_out,
            C_out,
            H_out,
            W_out));

    MIOPEN_ENFORCE(miopenPoolingGetWorkSpaceSize(
            top_desc_,
            &poolWsSize_));

    if((poolWsSize_ > 0) && (poolWs_ == nullptr)) {
      HIP_CHECK(hipMalloc(&poolWs_, poolWsSize_));
    }

    const T* Xdata = X.template data<T>();
    T* Ydata = Y->template mutable_data<T>();
    MIOPEN_ENFORCE(miopenPoolingForward(
        miopen_wrapper_.inline_miopen_handle(),
        pooling_desc_,
        &alpha_,
        bottom_desc_,
        Xdata,
        &beta_,
        top_desc_,
        Ydata,
        do_backward_,
        poolWs_,
        poolWsSize_));

    return true;
  }

  bool RunOnDevice() final {
    auto& X = Input(0);
    auto* Y = Output(0);
//TODO enable fp16
    if (X.IsType<float>()) {
      return DoRunWithType<float, float>();
    } else {
      LOG(FATAL) << "Unsupported input types";
    }
    return true;
  }

 protected:
  size_t poolWsSize_;
  MIOPENWrapper miopen_wrapper_;
  miopenTensorDescriptor_t bottom_desc_;
  miopenTensorDescriptor_t top_desc_;
  miopenPoolingDescriptor_t pooling_desc_;
  miopenPoolingMode_t mode_;
  bool do_backward_;
  const float alpha_;
  const float beta_;
private:
};

class MIOPENPoolGradientOp : public ConvPoolOpBase<HIPContext> {
 public:
  MIOPENPoolGradientOp(const OperatorDef& operator_def, Workspace* ws)
      : ConvPoolOpBase<HIPContext>(operator_def, ws),
        miopen_wrapper_(&context_),
        alpha_(OperatorBase::GetSingleArgument<float>("alpha", 1.0)),
        beta_(OperatorBase::GetSingleArgument<float>("beta", 0.0)),
        poolWsSize_(0)
  {
    MIOPEN_ENFORCE(miopenCreateTensorDescriptor(&bottom_desc_));
    MIOPEN_ENFORCE(miopenCreateTensorDescriptor(&top_desc_));
    MIOPEN_ENFORCE(miopenCreatePoolingDescriptor(&pooling_desc_));

    if (operator_def.type().substr(0, 7) == "MaxPool") {
      mode_ = miopenPoolingMax;
    }
    else if (operator_def.type().substr(0, 11) == "AveragePool") {
      mode_ =  miopenPoolingAverage;
    }
    else
    {
      LOG(FATAL) << "Unsupported pooling method: " << operator_def.type();
    }
  }

  ~MIOPENPoolGradientOp() {
    MIOPEN_ENFORCE(miopenDestroyTensorDescriptor(bottom_desc_));
    MIOPEN_ENFORCE(miopenDestroyTensorDescriptor(top_desc_));
    MIOPEN_ENFORCE(miopenDestroyPoolingDescriptor(pooling_desc_));
    poolWsSize_ = 0;
  }

  template <typename T, typename M>
  bool DoRunWithType() {
    auto& X = Input(0);
    auto& Y = Input(1);
    auto& dY = Input(2);
    auto* dX = Output(0);

    // cuDNN pooling support only 2 and 3 spatial dimensions.
    CAFFE_ENFORCE(X.ndim() >= 4 && X.ndim() <= 5);

    dX->ResizeLike(X);
    int N = 0, C = 0, H = 0, W = 0, D = 0;
    int N_out = 0, C_out = 0, H_out = 0, W_out = 0, D_out = 0;
    N = X.dim32(0);
    C = X.dim32(1);
    H = X.dim32(2);
    W = X.ndim() > 3 ? X.dim32(3) : 1;
    D = X.ndim() > 4 ? X.dim32(4) : 1;
    N_out = Y.dim32(0);
    C_out = Y.dim32(1);
    H_out = Y.dim32(2);
    W_out = Y.ndim() > 3 ? Y.dim32(3) : 1;
    D_out = Y.ndim() > 4 ? Y.dim32(4) : 1;

    MIOPEN_ENFORCE(miopenSet2dPoolingDescriptor(
            pooling_desc_,
            mode_,
            kernel_h(),
            kernel_w(),
            pad_t(),
            pad_l(),
            stride_h(),
            stride_w()));

    MIOPEN_ENFORCE(miopenSet4dTensorDescriptor(
            bottom_desc_,
            miopenTypeWrapper<T>::type,
            N,
            C,
            H,
            W));

    MIOPEN_ENFORCE(miopenSet4dTensorDescriptor(
            top_desc_,
            miopenTypeWrapper<T>::type,
            N_out,
            C_out,
            H_out,
            W_out));

    MIOPEN_ENFORCE(miopenPoolingGetWorkSpaceSize(
            top_desc_,
            &poolWsSize_));

    if((poolWsSize_ > 0) && (poolWs_ == nullptr)) {
      HIP_CHECK(hipMalloc(&poolWs_, poolWsSize_));
    }

    // Carry out the pooling computation.
    const T* Xdata = X.template data<T>();
    const T* Ydata = Y.template data<T>();
    const T* dYdata = dY.template data<T>();
    T* dXdata = dX->template mutable_data<T>();

    MIOPEN_ENFORCE(miopenPoolingBackward(
        miopen_wrapper_.inline_miopen_handle(),
        pooling_desc_,
        &alpha_,
        top_desc_,
        Ydata,
        top_desc_,
        dYdata,
        bottom_desc_,
        Xdata,
        &beta_,
        bottom_desc_,
        dXdata,
        poolWs_));

    return true;
  }

  bool RunOnDevice() final {
    auto& X = Input(0);
    auto& Y = Input(1);
    auto& dY = Input(2);
    auto* dX = Output(0);
    dX->ResizeLike(X);

    if (X.IsType<float>()) {
      return DoRunWithType<float, float>();
    } else {
      LOG(FATAL) << "Unsupported input types";
    }
    return true;
  }

 protected:
  size_t poolWsSize_;
  MIOPENWrapper miopen_wrapper_;
  miopenTensorDescriptor_t bottom_desc_;
  miopenTensorDescriptor_t top_desc_;
  miopenPoolingDescriptor_t pooling_desc_;
  miopenPoolingMode_t mode_;
  const float alpha_;
  const float beta_;
};

namespace {
REGISTER_MIOPEN_OPERATOR(AveragePool2D, MIOPENPoolOp);
REGISTER_MIOPEN_OPERATOR(AveragePool2DGradient, MIOPENPoolGradientOp);

REGISTER_MIOPEN_OPERATOR(MaxPool2D, MIOPENPoolOp);
REGISTER_MIOPEN_OPERATOR(MaxPool2DGradient, MIOPENPoolGradientOp);
} // namespace
} // namespace caffe2
