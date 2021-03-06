#include "saber/core/context.h"
#include "saber/funcs/fc.h"
#include "saber/core/tensor_op.h"
#include "saber/saber_types.h"
#include "test_saber_func.h"
#include "test_saber_base.h"
#include <vector>
#include <ctime>

using namespace anakin::saber;


//fc compute (native cpu version)
template <typename dtype, typename TargetType_D, typename TargetType_H>
void fc_cpu_base(const std::vector<Tensor<TargetType_H>* >& input,
                 std::vector<Tensor<TargetType_H>* >& output, \
                 FcParam<TargetType_D>& param) {

    const dtype* data_in = (const dtype*)input[0]->data();
    const dtype* bias = param.bias ? (const dtype*)param.bias->data() : nullptr;

    Tensor<TargetType_H> weights_h(param.weights->valid_shape());
    weights_h.copy_from(*param.weights);

    const dtype* weights = (const dtype*)weights_h.data();
    dtype* data_out = (dtype*)output[0]->mutable_data();

    //is_trans: flase.
    //output: data_out; inputs: data_in ; weights: weights.
    //data_out = data_in * weights. Get weights' elements continuosly.
    int out_rows = input[0]->num();
    int in_cols = input[0]->valid_size() / out_rows;
    int out_cols = param.weights->valid_size() / in_cols;
    int index_out;

    for (int i = 0; i < out_rows; i++) {
        for (int j = 0; j < out_cols; j++) {
            index_out = i * out_cols + j;
            data_out[index_out] = bias ? bias[j] : 0;

            for (int k = 0; k < in_cols; k++) {
                //data_out[index_out] += data_in[i * in_cols + k] * weights[k * out_cols + j];
                data_out[index_out] += data_in[i * in_cols + k] * weights[j * in_cols + k];
            }
        }
    }
}

TEST(TestSaberFunc, test_op_fc) {

#ifdef USE_CUDA
    TestSaberBase<NV, NVHX86, AK_FLOAT, Fc, FcParam> testbase;

    Tensor<NVHX86> weights_h;
    Tensor<NV> weights_d;

    //Shape shape_weight({})
    for (int w_in : {
                2, 8, 16
            }) {
        for (int h_in : {
                    2, 8, 32
                }) {
            for (int ch_in : {
                        2, 3, 8, 64
                    }) {
                for (int num_in : {
                            1, 21, 32
                        }) {
                    int out_num = w_in * 2;
                    Shape shape({num_in, ch_in, h_in, w_in});
                    Shape shape_w({ch_in, h_in, w_in, out_num});
                    weights_h.re_alloc(shape_w, AK_FLOAT);
                    weights_d.re_alloc(shape_w, AK_FLOAT);
                    fill_tensor_rand(weights_h, 0.1, 1.5);
                    weights_d.copy_from(weights_h);
                    FcParam<NV> param(&weights_d, out_num);
                    testbase.set_param(param);
                    testbase.set_rand_limit(1, 12);
                    testbase.set_input_shape(shape);
                    testbase.run_test(fc_cpu_base<float, NV, NVHX86>, 2.1e-5f);
                }
            }
        }
    }

#endif

#ifdef USE_X86_PLACE
    TestSaberBase<X86, X86, AK_FLOAT, Fc, FcParam> testbase0;

    Tensor<X86> weights_h0;

    //Shape shape_weight({})
    for (int w_in : {
                2, 8, 16
            }) {
        for (int h_in : {
                    2, 8, 32
                }) {
            for (int ch_in : {
                        2, 3, 8, 64
                    }) {
                for (int num_in : {
                            1, 21, 32
                        }) {
                    int out_num = w_in * 2;
                    Shape shape({num_in, ch_in, h_in, w_in});
                    Shape shape_w({ch_in, h_in, w_in, out_num});
                    weights_h0.re_alloc(shape_w, AK_FLOAT);
                    fill_tensor_rand(weights_h0, 0.1, 1.5);
                    FcParam<X86> param(&weights_h0, out_num);
                    testbase0.set_param(param);
                    testbase0.set_rand_limit(-12, 12);
                    testbase0.set_input_shape(shape);
                    testbase0.run_test(fc_cpu_base<float, X86, X86>, 1.0e-3f);
                }
            }
        }
    }
#endif

#ifdef USE_ARM_PLACE
    TestSaberBase<ARM, ARM, AK_FLOAT, Fc, FcParam> testbase1;

    Tensor<ARM> weights_h1;

    for (int w_in : {
                2, 8, 16
            }) {
        for (int h_in : {
                    2, 8, 32
                }) {
            for (int ch_in : {
                        2, 3, 8
                    }) {
                for (int num_in : {
                            1, 2, 16
                        }) {
                    int out_num = w_in * 2;
                    //printf("w_in, h_in, ch_in, num_in, out_num: %d, %d, %d, %d, %d\n", w_in, h_in, ch_in, num_in, out_num);
                    Shape shape({num_in, ch_in, h_in, w_in});
                    Shape shape_w({ch_in, h_in, w_in, out_num});
                    weights_h1.re_alloc(shape_w, AK_FLOAT);
                    fill_tensor_rand(weights_h1, 0.1, 1.5);
                    FcParam<ARM> param(&weights_h1, out_num);
                    testbase1.set_param(param);
                    testbase1.set_rand_limit(-12, 12);
                    testbase1.set_input_shape(shape);
                    testbase1.run_test(fc_cpu_base<float, ARM, ARM>, 1.0e-3f);
                }
            }
        }
    }
#endif

#ifdef USE_MLU
    Env<MLU>::env_init();
    Env<MLUHX86>::env_init();
    TestSaberBase<MLU, MLUHX86, AK_FLOAT, Fc, FcParam> testbase;

    Tensor<MLUHX86> weights_h;
    Tensor<MLU> weights_d;

    //Shape shape_weight({})
    for (int w_in : {
                2, 8, 16
            }) {
        for (int h_in : {
                    2, 8, 32
                }) {
            for (int ch_in : {
                        2, 3, 8, 64
                    }) {
                for (int num_in : {
                            1, 21, 32
                        }) {
                    int out_num = w_in * 2;
                    Shape shape({num_in, ch_in, h_in, w_in});
                    Shape shape_w({ch_in, h_in, w_in, out_num});
                    weights_h.re_alloc(shape_w, AK_FLOAT);
                    weights_d.re_alloc(shape_w, AK_FLOAT);
                    fill_tensor_rand(weights_h, 0.1, 0.15);
                    weights_d.copy_from(weights_h);
                    FcParam<MLU> param(&weights_d, out_num);
                    testbase.set_param(param);
                    testbase.set_rand_limit(1, 12);
                    testbase.set_input_shape(shape);
                    testbase.run_test(fc_cpu_base<float, MLU, MLUHX86>, 0.02, true);
                }
            }
        }
    }
#endif  // USE_MLU
}

int main(int argc, const char** argv) {
    //!initial logger
    logger::init(argv[0]);
    InitTest();
    RUN_ALL_TESTS(argv[0]);
    return 0;
}