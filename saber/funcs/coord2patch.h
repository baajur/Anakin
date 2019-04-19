/* Copyright (c) 2018 Anakin Authors, Inc. All Rights Reserved.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef ANAKIN_SABER_FUNCS_COORD2PATCH_H
#define ANAKIN_SABER_FUNCS_COORD2PATCH_H

#include "saber/funcs/base.h"
#include "saber/funcs/impl/impl_coord2patch.h"

namespace anakin {

namespace saber {

template<typename TargetType,
        DataType OpDtype>
class Coord2Patch : public BaseFunc<
        TargetType,
        OpDtype,
        ImplBase,
        Coord2PatchParam
> {
public:
    using BaseFunc<
            TargetType,
            OpDtype,
            ImplBase,
            Coord2PatchParam>::BaseFunc;

    Coord2Patch() = default;

    typedef Tensor<TargetType> InDataTensor;
    typedef Tensor<TargetType> OutDataTensor;
    typedef Tensor<TargetType> OpTensor;
    typedef Coord2PatchParam<TargetType> Param_t;
    typedef std::vector<InDataTensor *> Input_v;
    typedef std::vector<OutDataTensor *> Output_v;
    typedef std::vector<Shape> Shape_v;

    virtual SaberStatus compute_output_shape(const Input_v& input, Output_v& output, \
        Param_t& param) override {
        CHECK_GT(input.size(), 1) << "coord2patch need 2 inputs";
        Shape output_shape = input[1]->valid_shape();
        output_shape[2] = param.output_h;
        output_shape[3] = param.output_w;
        output[0]->set_shape(output_shape);
        return SaberSuccess;
    }

    virtual SaberStatus init_impl(ImplEnum implenum) override {
        switch (implenum) {
            case VENDER_IMPL:
                this->_impl.push_back(new VenderCoord2Patch <TargetType, OpDtype>);
                return SaberSuccess;

            case SABER_IMPL:
                this->_impl.push_back(new SaberCoord2Patch <TargetType, OpDtype>);
                return SaberSuccess;

            default:
                return SaberUnImplError;
        }
    }

private:

    virtual void pick_best_static() override {
        if (true) // some condition?
            this->_best_impl = this->_impl[0];
    }

    //virtual void pick_best_runtime(Input_v input, Output_v output, Param_t& param) override {}

    virtual void pick_best_specify(ImplEnum implenum) override {
        this->_best_impl = this->_impl[0];
    }

};

}
}

#endif //ANAKIN_SABER_FUNCS_COORD2PATCH_H
