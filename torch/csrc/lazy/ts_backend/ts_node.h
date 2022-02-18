#pragma once

#include <c10/util/ArrayRef.h>
#include <torch/csrc/lazy/backend/lowering_context.h>
#include <torch/csrc/lazy/core/shape.h>
#include <torch/csrc/lazy/core/ir.h>
#include <torch/csrc/jit/ir/ir.h>
#include <torch/csrc/jit/api/function_impl.h>
#include <torch/csrc/lazy/ts_backend/ts_lowering_context.h>

namespace torch {
namespace lazy {

using TSOpVector = std::vector<torch::jit::Value*>;

// Helper that makes it easy to access the TsNode::shape() method
// from an torch::lazy::Output* that holds a Node* that points to a TsNode
// TODO(whc) remove these once migrating to codegen and cleaning up Shape use
TORCH_API const Shape& GetShapeFromTsOutput(const Output& output);
TORCH_API const Shape& GetShapeFromTsValue(const Value& value);
TORCH_API void TsNodeSetShapeDeferred(
    NodePtr node, const std::function<Shape()>& shape_fn);

class TORCH_API TsNode : public lazy::Node {
 public:
  TsNode(OpKind op, OpList operands, std::vector<Shape>&& shapes,
         size_t num_outputs = 1, hash_t hash_seed = kHashSeed);

  // Same as the constructor above, but the shape is generated by a function,
  // only if needed (shape cache miss).
  TsNode(OpKind op, OpList operands,
         const std::function<Shape()>& shape_fn,
         size_t num_outputs = 1, hash_t hash_seed = kHashSeed);

  // The shape is set later.
  TsNode(OpKind op, OpList operands, size_t num_outputs = 1,
         hash_t hash_seed = kHashSeed);

  void SetShapeDeferred(const std::function<Shape()>& shape_fn);

  // Contructor used to create leaf nodes.
  TsNode(OpKind op, Shape shape, size_t num_outputs = 1,
         hash_t hash_seed = kHashSeed);

  ~TsNode() override = default;

  Shape GetOpShape(
      const std::function<Shape()>& shape_fn) const;

  // Retrieves the full shape of the IR Node.
  c10::ArrayRef<Shape> shapes() const { return shapes_; }

  // Retrieves the shape of the output at a given index.
  const Shape& shape(size_t output_index = 0) const;

  std::string ToString() const override;

  static hash_t GetOpHash(OpKind op, const Shape& shape, hash_t hash_seed, bool bakeInSizes);

  const std::vector<Output>& operands() const override {
    return operands_as_outputs_;
  }
  const Output& operand(size_t i) const override {
    return operands_as_outputs_.at(i);
  }

  // Lower is a backend-specific method since it returns a backend specific
  // type. hence, it is convenient to define it differently per-backend rather
  // than at Node API
  virtual TSOpVector Lower(std::shared_ptr<torch::jit::GraphFunction> function,
                           TSLoweringContext* loctx) const;

 private:
  // Adds node's index output number as operand.
  void AddOperand(NodePtr node, size_t index = 0);

  std::vector<Shape> shapes_;
  // A node holds a real reference to its operands.
  std::vector<NodePtr> operands_;
  // Outputs do not hold references on the nodes, and neither do the uses, since
  // otherwise we get into circular reference counting.
  std::vector<Output> operands_as_outputs_;
};

}  // namespace lazy
}  // namespace torch
