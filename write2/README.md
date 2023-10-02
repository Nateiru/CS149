# [Write Assignment 2](https://gfxcourses.stanford.edu/cs149/fall21content/static/pdfs/written_asst2.pdf)
[TOC]

## Problem 1: SPMD Tree Search

类似线段树，父亲的区间一定包含儿子节点的区间，但是叶子结点的区间可能和兄弟区间有重叠。会把题目所给的区间建成树形结构，并且每个区间用唯一的叶子节点表示，非叶子节点的区间仅用于维护树的结构，没有代表任何区间。

> 问题：每次给出一个点的坐标，要求找到范围最大的区间包含该点。

当输出的点 p=0.1 时，函数 `find_segment_1` 和 `find_segment_1` 都是左子树优先 DFS 遍历：
(I-test,N0), (I-hit,N0), (I-test, N1), (I-hit, N1), (I-test, N2), (I-hit, N2), (S-test,N3), (S-hit, N3), (I-test, N4), (I-hit, N4), (S-test, N5), (S-hit, N5), (S-test, N6), (S-test,N7), (I-test, N8), (I-miss, N8)

```cpp
struct Node {
  float min, max; // if leaf: start/end of segment, else: bounds on all child segments.
  bool leaf; // true if nodes is a leaf node
  int segment_id; // segment id if this is a leaf
  Node *left, *right; // child tree nodes
};

// -- computes segment id of the largest segment containing points[programIndex]
// -- root_node is the root of the search tree
// -- each program instance processes one query point
export void find_segment_1(uniform float *points, uniform int *results, uniform Node *root_node) {
  Stack<Node *> stack;
  Node *node;
  float max_extent = 0.0;

  // p is the point this program instance is searching for
  float p = points[programIndex];
  results[programIndex] = NO_SEGMENT;
  stack.push(root_node);

  while (!stack.size() == 0) {
    node = stack.pop();

    while (!node->leaf) {
      // [I-test]: test to see if the point is contained within this interior node
      if (p >= node->min && p <= node->max) {
        // [I-hit]: p is within an interior node... continue to child nodes
        push(node->right);
        node = node->left;
      } else {
        // [I-miss]: point not contained within the node, pop the stack
        if (stack.size() == 0)
          return;
        else
          node = stack.pop();
      }
    }

    // [S-test]: test if the point is within a segment, and the segment is the largest seen so far
    if (p >= node->min && p <= node->max && (node->max - node->min) > max_extent) {
      // [S-hit]: mark this segment as ‘‘best-so-far’’
      results[programIndex] = node->segment_id;
      max_extent = node->max - node->min;
    }
  }
}

```

|步骤| 0.15  | 0.35 | 0.75 | 0.95 |
| :----: |  :----:  |  :----:  |  :----:  |  :----:  |
| step1 | (I-test, N0) | (I-test, N0) | (I-test, N0) | (I-test, N0) |
| step2 | (I-hit, N0) | (I-hit, N0) | (I-hit, N0) | (I-hit, N0) |
| step3 | (I-test, N1) | (I-test, N1) | (I-test, N1) | (I-test, N1) |
| step4 | (I-hit, N1) | (I-hit, N1) | (I-miss, N1) | (I-miss, N1|
| step5 | (I-test, N2) | (I-test, N2) | (I-test, N8) | (I-test, N8) |
| step6 | (I-hit, N2) | (I-miss, N2) | (I-hit, N8)  | (I-hit, N8) |
| step7 | (S-test, N3) | (S-test, N7) | (I-test, N9) | (I-test, N9) |
| step8 | $\color{Red}(\text{S-hit, N3})$ | $\color{Red}(\text{S-hit, N7})$ | (I-hit, N9) | (I-miss, N9) |
| step9 | (I-test, N4) | (I-test, N8) | (I-test, N10)  | (S-test, N14) |
| step10 | (I-hit, N4) | (I-miss, N8) | (I-hit, N10)  | $\color{Red}(\text{S-hit, N14})$ |
| step11 | (S-test, N5) |  | (S-test, N11) |  |
| step12 | $\color{Red}(\text{S-hit, N5})$ |  |$\color{Red}(\text{S-hit, N11})$  |  |
| step13 | (S-test, N6) |  | (S-test, N12) |  |
| step14 | (S-test,N7) |  | $\color{Red}(\text{S-hit, N12})$ |  |
| step15 | (I-test, N8) |  | (S-test, N13) |  |
| step16 | (I-miss, N8) |  | $\color{Red}(\text{S-hit, N13})$ |  |
| step17 | | | (S-test, N14) |  |

```cpp

export void find_segment_2(uniform float *points, uniform int *results, uniform Node *root_node) {
  Stack<Node *> stack;
  Node *node;
  float max_extent = 0.0;
  // p is the point this program instance is searching for
  float p = points[programIndex];
  results[programIndex] = NO_SEGMENT;
  stack.push(root_node);

  while (!stack.size() == 0) {
    node = stack.pop();

    if (!node->leaf) {
      // [I-test]: test to see if the point is contained within an interior node
      if (p >= node->min && p <= node->max) {
        // [I-hit]: the point is within an interior node... continue to child nodes
        push(node->right);
        push(node->left);
      }
    } else {
      // [S-test]: test if the point is within a segment, and the segment is the largest seen so far
      if (p >= node->min && p <= node->max && (node->max - node->min) > max_extent) {
        // [S-hit]: mark this segment as the 'best-so-far'
        results[programIndex] = node->segment_id;
        max_extent = node->max - node->min;
      }
    }
  }
}
```

|步骤| 0.15  | 0.35 | 0.75 | 0.95 |
| :----: |  :----:  |  :----:  |  :----:  |  :----:  |
| step1 | (I-test, N0) | (I-test, N0) | (I-test, N0) | (I-test, N0) |
| step2 | (I-hit, N0) | (I-hit, N0) | (I-hit, N0) | (I-hit, N0) |
| step3 | (I-test, N1) | (I-test, N1) | (I-test, N1) | (I-test, N1) |
| step4 | (I-hit, N1) | (I-hit, N1) | (I-test, N8) | (I-test, N8|
| step5 | (I-test, N2) | (I-test, N2) | (I-hit, N8) | (I-hit, N8) |
| step6 | (I-hit, N2) | (S-test, N7) | (I-test, N9)  | (I-test, N9) |
| step7 | (S-test, N3) | $\color{Red}(\text{S-hit, N7})$ | (I-hit, N9) | (S-test, N14)|
| step8 | $\color{Red}(\text{S-hit, N3})$ | (I-test, N8) | (I-test, N10) | $\color{Red}(\text{S-hit, N14})$|
| step9 | (I-test, N4) | | (I-hit, N10)  | |
| step10 | (I-hit, N4) | | (S-test, N11)  | |
| step11 | (S-test, N5) |  | $\color{Red}(\text{S-hit, N11})$ |  |
| step12 | $\color{Red}(\text{S-hit, N5})$ |  |(S-test, N12)  |  |
| step13 | (S-test, N6) |  | $\color{Red}(\text{S-hit, N12})$ |  |
| step14 | (S-test,N7) |  | (S-test, N13) |  |
| step15 | (I-test, N8) |  | $\color{Red}(\text{S-hit, N13})$ |  |
| step16 | |  | (S-test, N14) |  |

### A
在 find_segment_2 没有 [I-miss] 的过程，利用循环巧妙的规避这个消耗。
### B
find_segment_1 更合适这个例子，[I-miss] 的过程使得 [S-hit] 在 SIMD 的情况下更多的重叠，比如在 step8 和 step12 同时都有在执行 [S-hit] 的过程，而在 find_segment_1 仅仅在 step8 时同时执行 [S-hit]，更具体的两个函数在本数据下执行指令的情况：

- `find_segment_1`: 12 × 其他指令 + 5 × [S-hit]
- `find_segment_2`: 10 × 其他指令 + 6 × [S-hit]

由于 [S-hit] 更加费时，因此 find_segment_1 比较合适。

## Problem 2: A Cardinal Processor Pipeline
