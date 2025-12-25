import time
from typing import Optional, List, Tuple, Dict

class BSTNode:
    """Node for Binary Search Tree"""
    def __init__(self, postid: str, timestamp: int, score: int):
        self.postid = postid
        self.timestamp = timestamp  # BST key
        self.score = score
        self.left: Optional['BSTNode'] = None
        self.right: Optional['BSTNode'] = None
    
    def __repr__(self):
        return f"Node({self.postid}, ts={self.timestamp}, score={self.score})"


class BST:
    """
    Binary Search Tree
    - BST property: ordered by timestamp
    - No automatic balancing (unlike Treap)
    - Score updates don't affect structure
    """
    def __init__(self):
        self.root: Optional[BSTNode] = None
        self.id_map: Dict[str, BSTNode] = {}  # postid -> Node mapping
        
        # Metrics
        self.insert_time = 0
        self.delete_time = 0
        self.search_time = 0
        self.insert_ops = 0
        self.delete_ops = 0
        self.search_ops = 0
    
    def addPost(self, postid: str, timestamp: int, score: int) -> bool:
        """Add a new post. Returns True if added, False if duplicate."""
        if postid in self.id_map:
            return False
        
        start = time.perf_counter()
        
        new_node = BSTNode(postid, timestamp, score)
        self.id_map[postid] = new_node
        
        # Iterative insertion to avoid stack overflow
        if self.root is None:
            self.root = new_node
        else:
            current = self.root
            while True:
                # BST insertion by timestamp (then postid for ties)
                if timestamp < current.timestamp or (timestamp == current.timestamp and postid < current.postid):
                    if current.left is None:
                        current.left = new_node
                        break
                    current = current.left
                elif timestamp > current.timestamp or (timestamp == current.timestamp and postid > current.postid):
                    if current.right is None:
                        current.right = new_node
                        break
                    current = current.right
                else:
                    # Duplicate - remove from id_map and return False
                    del self.id_map[postid]
                    self.insert_time += time.perf_counter() - start
                    return False
        
        self.insert_time += time.perf_counter() - start
        self.insert_ops += 1
        return True
    
    def likePost(self, postid: str) -> bool:
        """
        Increment score. 
        NOTE: In BST, this does NOT affect tree structure.
        Returns True if found.
        """
        start = time.perf_counter()
        
        node = self.id_map.get(postid)
        if not node:
            self.search_time += time.perf_counter() - start
            return False
        
        # Simply increment score - no restructuring needed
        node.score += 1
        
        self.search_time += time.perf_counter() - start
        self.search_ops += 1
        return True
    
    def deletePost(self, postid: str) -> bool:
        """Delete a post. Returns True if deleted, False if not found."""
        start = time.perf_counter()
        
        node = self.id_map.get(postid)
        if not node:
            self.delete_time += time.perf_counter() - start
            return False
        
        timestamp = node.timestamp
        
        # Iterative delete to avoid stack overflow
        parent = None
        current = self.root
        is_left_child = False
        
        # Find the node and its parent
        while current:
            if timestamp < current.timestamp or (timestamp == current.timestamp and postid < current.postid):
                parent = current
                current = current.left
                is_left_child = True
            elif timestamp > current.timestamp or (timestamp == current.timestamp and postid > current.postid):
                parent = current
                current = current.right
                is_left_child = False
            else:
                # Found the node to delete
                break
        
        if not current:
            self.delete_time += time.perf_counter() - start
            return False
        
        # Case 1: Node has no children
        if not current.left and not current.right:
            if parent is None:
                self.root = None
            elif is_left_child:
                parent.left = None
            else:
                parent.right = None
        
        # Case 2: Node has only right child
        elif not current.left:
            if parent is None:
                self.root = current.right
            elif is_left_child:
                parent.left = current.right
            else:
                parent.right = current.right
        
        # Case 3: Node has only left child
        elif not current.right:
            if parent is None:
                self.root = current.left
            elif is_left_child:
                parent.left = current.left
            else:
                parent.right = current.left
        
        # Case 4: Node has two children
        else:
            # Find inorder successor (minimum in right subtree)
            successor_parent = current
            successor = current.right
            while successor.left:
                successor_parent = successor
                successor = successor.left
            
            # Copy successor data to current node
            current.postid = successor.postid
            current.timestamp = successor.timestamp
            current.score = successor.score
            self.id_map[current.postid] = current
            
            # Delete successor
            if successor_parent == current:
                successor_parent.right = successor.right
            else:
                successor_parent.left = successor.right
        
        del self.id_map[postid]
        
        self.delete_time += time.perf_counter() - start
        self.delete_ops += 1
        return True
    
    def _find_max_score_node(self, node: Optional[BSTNode], 
                             current_max: Optional[BSTNode]) -> Optional[BSTNode]:
        """Traverse tree to find node with maximum score"""
        if not node:
            return current_max
        
        # Check if current node has higher score
        if current_max is None or node.score > current_max.score or \
           (node.score == current_max.score and node.timestamp < current_max.timestamp):
            current_max = node
        
        # Check both subtrees
        left_max = self._find_max_score_node(node.left, current_max)
        right_max = self._find_max_score_node(node.right, left_max)
        
        return right_max
    
    def getMostPopular(self) -> Optional[Tuple[str, int, int]]:
        """
        Get post with highest score.
        NOTE: In BST, this requires O(n) traversal since score doesn't determine structure.
        """
        start = time.perf_counter()
        
        max_node = self._find_max_score_node(self.root, None)
        
        self.search_time += time.perf_counter() - start
        self.search_ops += 1
        
        if not max_node:
            return None
        return (max_node.postid, max_node.timestamp, max_node.score)
    
    def _get_recent(self, node: Optional[BSTNode], k: int, 
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
    
    def _height(self, node: Optional[BSTNode]) -> int:
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
            'rotations': 0,  # BST doesn't rotate
            'avg_insert_time_ms': (self.insert_time / self.insert_ops * 1000) if self.insert_ops > 0 else 0,
            'avg_delete_time_ms': (self.delete_time / self.delete_ops * 1000) if self.delete_ops > 0 else 0,
            'avg_search_time_ms': (self.search_time / self.search_ops * 1000) if self.search_ops > 0 else 0,
            'total_operations': self.insert_ops + self.delete_ops + self.search_ops
        }
    
    def resetMetrics(self):
        """Reset performance counters"""
        self.insert_time = 0
        self.delete_time = 0
        self.search_time = 0
        self.insert_ops = 0
        self.delete_ops = 0
        self.search_ops = 0


# Test with PDF sample
def test_bst_sample():
    print("=" * 60)
    print("BST - Testing with PDF Sample")
    print("=" * 60)
    
    bst = BST()
    
    # 1. Add posts
    print("\n1. Adding posts...")
    bst.addPost("ejualnb", 1554076800, 55)
    bst.addPost("ejualnc", 1554076800, 12)
    bst.addPost("ejualnd", 1554076800, 27)
    bst.addPost("ejualne", 1554076800, 14)
    bst.addPost("ejualnl", 1554076809, 13)
    
    print(f"Tree size: {bst.getSize()}")
    print(f"Tree height: {bst.getHeight()}")
    print(f"Total rotations: 0 (BST doesn't rotate)")
    
    # 2. Like post 'ejualnl' twice
    print("\n2. Liking post 'ejualnl' twice...")
    bst.likePost("ejualnl")
    bst.likePost("ejualnl")
    print(f"Post 'ejualnl' new score: {bst.id_map['ejualnl'].score}")
    print(f"Note: BST structure unchanged by score updates")
    
    # 3. Delete post 'ejualnc'
    print("\n3. Deleting post 'ejualnc'...")
    bst.deletePost("ejualnc")
    print(f"Tree size: {bst.getSize()}")
    
    # 4. Get most popular (O(n) operation!)
    print("\n4. Most popular post (requires full traversal):")
    print(f"   {bst.getMostPopular()}")
    
    # 5. Get 3 most recent
    print("\n5. 3 most recent posts:")
    for i, post in enumerate(bst.getMostRecent(3), 1):
        print(f"   {i}. {post}")
    
    print("\n" + "=" * 60)


if __name__ == "__main__":
    test_bst_sample()