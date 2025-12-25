import time
from typing import Optional, List, Tuple, Dict

class TreapNode:
    """Node for Treap structure"""
    def __init__(self, postid: str, timestamp: int, score: int):
        self.postid = postid
        self.timestamp = timestamp  # BST key
        self.score = score          # Heap priority (max-heap)
        self.left: Optional['TreapNode'] = None
        self.right: Optional['TreapNode'] = None
    
    def __repr__(self):
        return f"Node({self.postid}, ts={self.timestamp}, score={self.score})"


class Treap:
    """
    Treap (Randomized Binary Search Tree)
    - BST property: ordered by timestamp
    - Max-heap property: ordered by score (higher score = higher in tree)
    """
    def __init__(self):
        self.root: Optional[TreapNode] = None
        self.id_map: Dict[str, TreapNode] = {}  # postid -> Node mapping
        
        # Metrics
        self.rotation_count = 0
        self.insert_time = 0
        self.delete_time = 0
        self.search_time = 0
        self.insert_ops = 0
        self.delete_ops = 0
        self.search_ops = 0
    
    def _has_higher_priority(self, n1: TreapNode, n2: TreapNode) -> bool:
        """Check if n1 has higher priority than n2 (for max-heap)"""
        if n1.score != n2.score:
            return n1.score > n2.score
        # Tie-breaker: older post wins
        return n1.timestamp < n2.timestamp
    
    def _rotate_right(self, node: TreapNode) -> TreapNode:
        """Right rotation around node"""
        self.rotation_count += 1
        left = node.left
        node.left = left.right
        left.right = node
        return left
    
    def _rotate_left(self, node: TreapNode) -> TreapNode:
        """Left rotation around node"""
        self.rotation_count += 1
        right = node.right
        node.right = right.left
        right.left = node
        return right
    
    def _insert(self, node: Optional[TreapNode], postid: str, 
                timestamp: int, score: int) -> TreapNode:
        """Recursive insert maintaining BST and heap properties"""
        if node is None:
            new_node = TreapNode(postid, timestamp, score)
            self.id_map[postid] = new_node
            return new_node
        
        # BST insertion by timestamp (then postid for ties)
        if timestamp < node.timestamp or (timestamp == node.timestamp and postid < node.postid):
            node.left = self._insert(node.left, postid, timestamp, score)
            # Fix heap property
            if self._has_higher_priority(node.left, node):
                node = self._rotate_right(node)
        elif timestamp > node.timestamp or (timestamp == node.timestamp and postid > node.postid):
            node.right = self._insert(node.right, postid, timestamp, score)
            # Fix heap property
            if self._has_higher_priority(node.right, node):
                node = self._rotate_left(node)
        # Else: duplicate, skip
        
        return node
    
    def addPost(self, postid: str, timestamp: int, score: int) -> bool:
        """Add a new post. Returns True if added, False if duplicate."""
        if postid in self.id_map:
            return False
        
        start = time.perf_counter()
        self.root = self._insert(self.root, postid, timestamp, score)
        self.insert_time += time.perf_counter() - start
        self.insert_ops += 1
        return True
    
    def likePost(self, postid: str) -> bool:
        """Increment score and rebalance. Returns True if found."""
        start = time.perf_counter()
        
        node = self.id_map.get(postid)
        if not node:
            self.search_time += time.perf_counter() - start
            return False
        
        # Remove and reinsert with new score
        old_timestamp = node.timestamp
        old_score = node.score
        self._delete_node(postid, old_timestamp)
        self.root = self._insert(self.root, postid, old_timestamp, old_score + 1)
        
        self.search_time += time.perf_counter() - start
        self.search_ops += 1
        return True
    
    def _delete(self, node: Optional[TreapNode], postid: str, 
                timestamp: int) -> Optional[TreapNode]:
        """Recursive delete by rotating down to leaf"""
        if not node:
            return None
        
        if timestamp < node.timestamp or (timestamp == node.timestamp and postid < node.postid):
            node.left = self._delete(node.left, postid, timestamp)
        elif timestamp > node.timestamp or (timestamp == node.timestamp and postid > node.postid):
            node.right = self._delete(node.right, postid, timestamp)
        else:
            # Found the node to delete
            if not node.left:
                return node.right
            if not node.right:
                return node.left
            
            # Two children: rotate down the child with lower priority
            if self._has_higher_priority(node.left, node.right):
                node = self._rotate_right(node)
                node.right = self._delete(node.right, postid, timestamp)
            else:
                node = self._rotate_left(node)
                node.left = self._delete(node.left, postid, timestamp)
        
        return node
    
    def _delete_node(self, postid: str, timestamp: int):
        """Helper to delete node from tree"""
        self.root = self._delete(self.root, postid, timestamp)
        if postid in self.id_map:
            del self.id_map[postid]
    
    def deletePost(self, postid: str) -> bool:
        """Delete a post. Returns True if deleted, False if not found."""
        start = time.perf_counter()
        
        node = self.id_map.get(postid)
        if not node:
            self.delete_time += time.perf_counter() - start
            return False
        
        self._delete_node(postid, node.timestamp)
        self.delete_time += time.perf_counter() - start
        self.delete_ops += 1
        return True
    
    def getMostPopular(self) -> Optional[Tuple[str, int, int]]:
        """Get post with highest score (root). O(1)"""
        if not self.root:
            return None
        return (self.root.postid, self.root.timestamp, self.root.score)
    
    def _get_recent(self, node: Optional[TreapNode], k: int, 
                    result: List[Tuple[str, int, int]]):
        """Reverse in-order traversal (newest first)"""
        if not node or len(result) >= k:
            return
        
        self._get_recent(node.right, k, result)
        if len(result) < k:
            result.append((node.postid, node.timestamp, node.score))
        self._get_recent(node.left, k, result)
    
    def getMostRecent(self, k: int) -> List[Tuple[str, int, int]]:
        """Get k most recent posts. O(k + log n)"""
        result = []
        self._get_recent(self.root, k, result)
        return result
    
    def _height(self, node: Optional[TreapNode]) -> int:
        """Calculate tree height"""
        if not node:
            return 0
        return 1 + max(self._height(node.left), self._height(node.right))
    
    def getHeight(self) -> int:
        """Get tree height"""
        return self._height(self.root)
    
    def getSize(self) -> int:
        """Get number of nodes"""
        return len(self.id_map)
    
    def getMetrics(self) -> Dict:
        """Get performance metrics"""
        return {
            'size': self.getSize(),
            'height': self.getHeight(),
            'rotations': self.rotation_count,
            'avg_insert_time_ms': (self.insert_time / self.insert_ops * 1000) if self.insert_ops > 0 else 0,
            'avg_delete_time_ms': (self.delete_time / self.delete_ops * 1000) if self.delete_ops > 0 else 0,
            'avg_search_time_ms': (self.search_time / self.search_ops * 1000) if self.search_ops > 0 else 0,
            'total_operations': self.insert_ops + self.delete_ops + self.search_ops
        }
    
    def resetMetrics(self):
        """Reset performance counters"""
        self.rotation_count = 0
        self.insert_time = 0
        self.delete_time = 0
        self.search_time = 0
        self.insert_ops = 0
        self.delete_ops = 0
        self.search_ops = 0
    
    # BONUS: Treap Union
    def union(self, other: 'Treap') -> 'Treap':
        """Merge two treaps into one"""
        result = Treap()
        
        def merge(n1: Optional[TreapNode], n2: Optional[TreapNode]) -> Optional[TreapNode]:
            if not n1:
                return self._clone_subtree(n2, result)
            if not n2:
                return self._clone_subtree(n1, result)
            
            if self._has_higher_priority(n1, n2):
                new_node = TreapNode(n1.postid, n1.timestamp, n1.score)
                result.id_map[n1.postid] = new_node
                
                left_part, right_part = self._split_by_key(n2, n1.timestamp, n1.postid)
                new_node.left = merge(n1.left, left_part)
                new_node.right = merge(n1.right, right_part)
                return new_node
            else:
                new_node = TreapNode(n2.postid, n2.timestamp, n2.score)
                result.id_map[n2.postid] = new_node
                
                left_part, right_part = self._split_by_key(n1, n2.timestamp, n2.postid)
                new_node.left = merge(left_part, n2.left)
                new_node.right = merge(right_part, n2.right)
                return new_node
        
        result.root = merge(self.root, other.root)
        return result
    
    def _split_by_key(self, node: Optional[TreapNode], timestamp: int, 
                      postid: str) -> Tuple[Optional[TreapNode], Optional[TreapNode]]:
        """Split tree: left < key, right >= key"""
        if not node:
            return None, None
        
        if node.timestamp < timestamp or (node.timestamp == timestamp and node.postid < postid):
            left, right = self._split_by_key(node.right, timestamp, postid)
            node.right = left
            return node, right
        else:
            left, right = self._split_by_key(node.left, timestamp, postid)
            node.left = right
            return left, node
    
    def _clone_subtree(self, node: Optional[TreapNode], target: 'Treap') -> Optional[TreapNode]:
        """Clone subtree into target treap"""
        if not node:
            return None
        
        new_node = TreapNode(node.postid, node.timestamp, node.score)
        target.id_map[node.postid] = new_node
        new_node.left = self._clone_subtree(node.left, target)
        new_node.right = self._clone_subtree(node.right, target)
        return new_node
    
    # BONUS: Treap Intersection
    def intersection(self, other: 'Treap') -> 'Treap':
        """Find common elements between two treaps"""
        result = Treap()
        
        def intersect(n1: Optional[TreapNode], n2: Optional[TreapNode]):
            if not n1 or not n2:
                return
            
            # Check if current node exists in other tree
            if n1.postid in other.id_map:
                node = other.id_map[n1.postid]
                result.addPost(node.postid, node.timestamp, node.score)
            
            # Continue traversal
            intersect(n1.left, n2)
            intersect(n1.right, n2)
        
        intersect(self.root, other.root)
        return result
    

    def resetMetrics(self):
        """Reset performance counters"""
        self.rotation_count = 0
        self.insert_time = 0
        self.delete_time = 0
        self.search_time = 0
        self.insert_ops = 0
        self.delete_ops = 0
        self.search_ops = 0
    
    def _verify_heap_property(self, node: Optional[TreapNode]) -> Tuple[bool, List[str]]:
        """
        Recursively verify that the max-heap property is maintained.
        Returns (is_valid, violations) where violations is a list of error messages.
        """
        violations = []
        
        if not node:
            return True, violations
        
        # Check left child
        if node.left:
            if not self._has_higher_priority(node, node.left):
                violations.append(
                    f"Heap violation: Parent {node.postid} (score={node.score}) "
                    f"has lower priority than left child {node.left.postid} (score={node.left.score})"
                )
        
        # Check right child
        if node.right:
            if not self._has_higher_priority(node, node.right):
                violations.append(
                    f"Heap violation: Parent {node.postid} (score={node.score}) "
                    f"has lower priority than right child {node.right.postid} (score={node.right.score})"
                )
        
        # Recursively check subtrees
        left_valid, left_violations = self._verify_heap_property(node.left)
        right_valid, right_violations = self._verify_heap_property(node.right)
        
        violations.extend(left_violations)
        violations.extend(right_violations)
        
        is_valid = len(violations) == 0
        return is_valid, violations
    
    def verifyHeapProperty(self) -> Tuple[bool, str]:
        """
        Verify that the treap maintains the max-heap property correctly.
        Returns (is_valid, message) tuple.
        """
        if not self.root:
            return True, "✓ Heap property verified: Tree is empty"
        
        is_valid, violations = self._verify_heap_property(self.root)
        
        if is_valid:
            return True, f"✓ Heap property verified: All {self.getSize()} nodes satisfy max-heap property"
        else:
            message = f"✗ Heap property VIOLATED: Found {len(violations)} violation(s)\n"
            message += "\n".join(f"  - {v}" for v in violations)
            return False, message


# Test with PDF sample
def test_treap_sample():
    print("=" * 60)
    print("TREAP - Testing with PDF Sample")
    print("=" * 60)
    
    treap = Treap()
    
    # 1. Add posts
    print("\n1. Adding posts...")
    treap.addPost("ejualnb", 1554076800, 55)
    treap.addPost("ejualnc", 1554076800, 12)
    treap.addPost("ejualnd", 1554076800, 27)
    treap.addPost("ejualne", 1554076800, 14)
    treap.addPost("ejualnl", 1554076809, 13)
    
    print(f"Tree size: {treap.getSize()}")
    print(f"Tree height: {treap.getHeight()}")
    print(f"Total rotations: {treap.rotation_count}")
    
    # 2. Like post 'ejualnl' twice
    print("\n2. Liking post 'ejualnl' twice...")
    treap.likePost("ejualnl")
    treap.likePost("ejualnl")
    print(f"Post 'ejualnl' new score: {treap.id_map['ejualnl'].score}")
    print(f"Total rotations: {treap.rotation_count}")
    
    # 3. Delete post 'ejualnc'
    print("\n3. Deleting post 'ejualnc'...")
    treap.deletePost("ejualnc")
    print(f"Tree size: {treap.getSize()}")
    print(f"Total rotations: {treap.rotation_count}")
    
    # 4. Get most popular
    print("\n4. Most popular post:")
    print(f"   {treap.getMostPopular()}")
    
    # 5. Get 3 most recent
    print("\n5. 3 most recent posts:")
    for i, post in enumerate(treap.getMostRecent(3), 1):
        print(f"   {i}. {post}")
    
    # 6. Test Union
    print("\n6. Testing Union...")
    treap2 = Treap()
    treap2.addPost("new1", 1554076850, 30)
    treap2.addPost("new2", 1554076860, 40)
    
    merged = treap.union(treap2)
    print(f"Merged treap size: {merged.getSize()}")
    
    # 7. Test Intersection
    print("\n7. Testing Intersection...")
    treap3 = Treap()
    treap3.addPost("ejualnb", 1554076800, 55)  # Common with treap
    treap3.addPost("unique", 1554076870, 20)
    
    common = treap.intersection(treap3)
    print(f"Intersection size: {common.getSize()}")
    
    print("\n" + "=" * 60)


if __name__ == "__main__":
    test_treap_sample()