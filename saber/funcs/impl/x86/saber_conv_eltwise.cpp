
#include "saber/funcs/impl/x86/saber_conv_eltwise.h"
#include "saber/funcs/calibrate.h"
#include "saber_conv_eltwise.h"

#include "saber/funcs/impl/x86/saber_im2col_conv.h"
#include "saber/funcs/impl/x86/kernel/jit_avx2_conv.h"
#include "saber/funcs/impl/x86/kernel/jit_uni_dwconv.h"
#include "saber/funcs/impl/x86/kernel/jit_avx512_conv1x1.h"
#include "saber/funcs/impl/x86/kernel/jit_avx512_conv.h"
#include "saber/funcs/impl/x86/gemm_x8s8s32x_conv.h"
#include "saber/funcs/impl/x86/saber_conv_1x1.h"
#include "saber/funcs/impl/x86/kernel/jit_uni_dwconv.h"


namespace anakin {
namespace saber {

template <DataType OpDtype>
SaberStatus SaberConvEltwise<X86, OpDtype>::trans_weights(Tensor<X86> &target_weights, Tensor<X86> &target_bias,
                          int pad_h, int pad_w, int dilation_h, int dilation_w,
                          int stride_h, int stride_w, int group){
    return SaberSuccess;
};
template <>
SaberStatus SaberConvEltwise<X86, AK_HALF>::trans_weights(Tensor<X86> &target_weights, Tensor<X86> &target_bias,
                                                           int pad_h, int pad_w, int dilation_h, int dilation_w,
                                                           int stride_h, int stride_w, int group){
    return SaberSuccess;
};
//template <>
//SaberStatus SaberConvEltwise<X86, AK_INT8>::trans_weights(Tensor<X86> &target_weights, Tensor<X86> &target_bias,
//                                                          int pad_h, int pad_w, int dilation_h, int dilation_w,
//                                                          int stride_h, int stride_w, int group){
//    return SaberSuccess;
//};

template <>
SaberStatus SaberConvEltwise<X86, AK_FLOAT>::\
        create(const std::vector<Tensor<X86> *>& inputs,
            std::vector<Tensor<X86> *>& outputs,
            ConvEltwiseParam<X86>& param, Context<X86>& ctx) {

    _ctx = &ctx;
    _inner_shape = conv_compute_shape(inputs[0]->valid_shape(), param.conv_param);

    //choose impl kernel
    bool use_avx512 = false;//mayiuse(avx512_common);
    bool use_avx2 = mayiuse(avx2);
    int group = param.conv_param.group;
    int oc = outputs[0]->channel();
    int ic = inputs[0]->channel();
    int kh = _kernel_height;
    int kw = _kernel_width;
    int pad_h = param.conv_param.pad_h;
    int pad_w = param.conv_param.pad_w;
    int stride_h = param.conv_param.stride_h;
    int stride_w = param.conv_param.stride_w;
    LayoutType input_layout = inputs[0]->get_layout();
    LayoutType out_layout = outputs[0]->get_layout();
    if (_do_in_impl){
        this->_impl->create(inputs, outputs, param, ctx);
    }

    return SaberSuccess;
}

template <>
SaberStatus SaberConvEltwise<X86, AK_FLOAT>::
    init(const std::vector<Tensor<X86> *>& inputs,
        std::vector<Tensor<X86> *>& outputs,
        ConvEltwiseParam<X86>& param, Context<X86>& ctx) {

    _ctx = &ctx;
    _inner_shape = conv_compute_shape(inputs[0]->valid_shape(), param.conv_param);
    _kernel_height = param.conv_param.weight()->height();
    _kernel_width = param.conv_param.weight()->width();

    //choose impl kernel
    bool use_avx512 = false;//mayiuse(avx512_common);
    bool use_avx2 = mayiuse(avx2);
    int group = param.conv_param.group;
    int oc = outputs[0]->channel();
    int ic = inputs[0]->channel();
    int kh = _kernel_height;
    int kw = _kernel_width;
    int pad_h = param.conv_param.pad_h;
    int pad_w = param.conv_param.pad_w;
    int stride_h = param.conv_param.stride_h;
    int stride_w = param.conv_param.stride_w;
    LayoutType input_layout = inputs[0]->get_layout();
    LayoutType out_layout = outputs[0]->get_layout();

    if ((kh == 1 && kw == 1) && (pad_h == 0 && pad_w == 0) && (stride_h == 1 && stride_w == 1) &&
            (input_layout == Layout_NCHW) && (out_layout == Layout_NCHW) && group == 1) {
        _do_in_impl = true;
        this->_impl = new SaberConv1X1<AK_FLOAT>;
        this->_impl->init(inputs, outputs, param, ctx);
    } else {
        _do_in_impl = false;
        _inner_tensor.re_alloc(_inner_shape, AK_FLOAT);
        _inner_tensor_v.resize(2);
        _inner_tensor_v[0] = &_inner_tensor;
        _conv.init(inputs, _inner_tensor_v, param.conv_param, ctx);
        _eltwise.init(_inner_tensor_v, outputs, param.eltwise_param, ctx);
    }
    //TODO:add some impl for eltwise
    /* 
    else if (use_avx2 && input_layout == Layout_NCHW_C8R && out_layout == Layout_NCHW_C8R
               && (oc == group && ic == group && oc % 8 == 0)) {
        this->_impl = new JitUniDWConv<AK_FLOAT>;
    } else if (use_avx512 && param.conv_param.group == inputs[0]->channel()
               && param.conv_param.group == outputs[0]->channel()) {
        this->_impl = new JitUniDWConv<AK_FLOAT>;
    } else if (use_avx512 && param.conv_param.weight()->height() == 1 
               && param.conv_param.weight()->width() == 1) {
        this->_impl = new JitAvx512Conv1x1<AK_FLOAT>;
    } else if (use_avx512 && outputs[0]->get_layout() == Layout_NCHW_C16) {
        this->_impl = new JitAvx512Conv<AK_FLOAT>;
    } else if (use_avx2 && param.conv_param.group == 1) {
        this->_impl = new JitAvx2Conv<AK_FLOAT>;
    } else if (input_layout == Layout_NCHW && out_layout == Layout_NCHW) {
        this->_impl = new SaberIm2colConv<AK_FLOAT>;
    } else {
        LOG(FATAL) << "not support conv for in shape = " << inputs[0]->valid_shape() << ", out shape "
                   << outputs[0]->valid_shape() << ", group = " << group;
    }
    */

    
    return create(inputs, outputs, param, ctx);
}

template <>
SaberStatus SaberConvEltwise<X86, AK_FLOAT>::dispatch(
        const std::vector<Tensor<X86>*>& inputs,
        std::vector<Tensor<X86>*>& outputs,
    ConvEltwiseParam<X86>& param) {
    
    if (_do_in_impl){
        _impl->dispatch(inputs, outputs, param);    
    } else {
        _conv.dispatch(inputs, _inner_tensor_v, param.conv_param);    
        _inner_tensor_v[1] = outputs[0];
        _eltwise.dispatch(_inner_tensor_v, outputs, param.eltwise_param);    
    }
    
    return SaberSuccess;
}
template <>
SaberStatus SaberConvEltwise<X86, AK_INT8>::\
create(const std::vector<Tensor<X86> *>& inputs,
       std::vector<Tensor<X86> *>& outputs,
       ConvEltwiseParam<X86>& param, Context<X86>& ctx) {
    this->_ctx = &ctx;

    return this->_impl->create(inputs, outputs, param, ctx);
}

template <>
SaberStatus SaberConvEltwise<X86, AK_INT8>::\
init(const std::vector<Tensor<X86> *>& inputs,
     std::vector<Tensor<X86> *>& outputs,
     ConvEltwiseParam<X86>& param, Context<X86>& ctx) {

    return this->_impl->init(inputs, outputs, param, ctx);
}

template <>
SaberStatus SaberConvEltwise<X86, AK_INT8>::\
dispatch(const std::vector<Tensor<X86> *>& inputs,
         std::vector<Tensor<X86> *>& outputs,
         ConvEltwiseParam<X86>& param) {
    return this->_impl->dispatch(inputs, outputs, param);
}

template class SaberConvEltwise<X86, AK_FLOAT>;
template class SaberConvEltwise<X86, AK_INT8>;
DEFINE_OP_TEMPLATE(SaberConvEltwise, ConvEltwiseParam, X86, AK_HALF);

}
}
