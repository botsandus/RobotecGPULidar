## Node operations

- Parameter modification
- Adding child-parent link
- Removing child-parent link
- Retrieving results
- Receiving Graph struct

## Graph operations

- Creating graph
- Starting graph

## Dependencies

- Adding, removing child-parent link
  - Requires updating Graph struct (e.g. execution order)
  - Requires updating cached inputs
  - two graphs merge


## Node data

Nodes can contain the following categories of data:
- Cached parameters (e.g. RaytraceNode::range)
- Handle(s) to validated input node(s) 
- State (e.g., results)
- Other resources (e.g., ROS2 topic publisher)
