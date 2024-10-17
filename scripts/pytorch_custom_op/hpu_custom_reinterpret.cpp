#include "hpu_custom_op.h"

#include <torch/extension.h>

#include <synapse_common_types.hpp>


bool register_custom_reinterpret() {
    // inputs desc
    habana::custom_op::InputDesc input_a_desc {
        habana::custom_op::input_type::TENSOR, 0
    };
    std::vector<habana::custom_op::InputDesc> inputs_desc { input_a_desc };

    auto output_size_lambda = [](const at::Stack& inputs) -> std::vector<int64_t> {
        return inputs[0].toTensor().sizes().vec(); // Output shape is same as input tensor shape
    };

    habana::custom_op::OutputDesc output_desc{
        0, c10::ScalarType::Float, output_size_lambda}; // Output dtype will be set in execute function
    std::vector<habana::custom_op::OutputDesc> outputs_desc{
        output_desc};
    // output desc
    /*habana::custom_op::OutputDesc output_desc {
        0, c10::ScalarType::Float // XXX if compute_output_shape_func is nullptr, it is assumed output has the same shape as input (read from hpu_custom_op.h)
    };
    std::vector<habana::custom_op::OutputDesc> outputs_desc { output_desc };*/

    // acctual register
    REGISTER_CUSTOM_OP_ATTRIBUTES(
        "custom_op::reinterpret_float", //schema name
        "reinterpret_fwd_i32", // guid XXX I assume this should be exactly the same as kernel name; passing for example 'relu_fwd_f32' works because this is a predefined op
        inputs_desc,
        outputs_desc,
        nullptr);
    std::cout << "cpp registered custom_op::reinterpret_float\n";
    return true;
}

at::Tensor custom_reinterpret_execute(
    torch::Tensor input_a) {
  // Registering the custom op, need to be called only once
  static bool registered = register_custom_reinterpret();
  TORCH_CHECK(registered, "custom_reinterpret kernel not registered" );
  std::vector<c10::IValue> inputs{input_a};
  // Get custom op descriptor from registry
  auto op_desc = habana::custom_op::HabanaCustomOpDescriptor::getCustomOpDescriptor("custom_op::reinterpret_float");
  if (op_desc.hasUserParamsFunc()) std::cout << "has user params func\n"; else std::cout << "does not have user params func\n";
  // Actual call for op execution
  std::cout << "about to call op_desc.execute()\n";
  std::vector<at::Tensor> output;
  try {
    output = op_desc.execute(inputs);
    std::cout << "Custom op executed successfully." << std::endl;
} catch (const std::exception& e) {
    std::cerr << "Custom op execution failed: " << e.what() << std::endl;
    // Handle the exception, possibly by rethrowing or returning early
}

    try {
    std::cout << output[0] << std::endl;;
    
    } catch (const std::exception& e) {
    std::cerr << "Failed to move tensor to CPU or print it: " << e.what() << std::endl;}
  //std::vector<at::Tensor> output = op_desc.execute(inputs); // XXX TODO The key is to know what is being done here. Is this calling getGcDefinitions etc?
  // op_desc.execute will always return a vector
  return output[0];
}

TORCH_LIBRARY(custom_op, m) {
  m.def("reinterpret_float(Tensor self) -> Tensor");
}
TORCH_LIBRARY_IMPL(custom_op, HPU, m) {
  m.impl("reinterpret_float", custom_reinterpret_execute);
}


