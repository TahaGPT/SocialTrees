import tkinter as tk
from tkinter import ttk, filedialog, messagebox, scrolledtext
import threading
import time
import csv
from pathlib import Path
from typing import Optional, List
import sys

# Import your tree implementations
from treap import Treap
from bst import BST


class TreeVisualizer(tk.Canvas):
    """Canvas for drawing tree structure"""
    
    def __init__(self, parent, **kwargs):
        super().__init__(parent, bg='white', **kwargs)
        self.node_radius = 25
        self.level_height = 80
        self.min_h_spacing = 60
        
    def draw_tree(self, tree, tree_type='treap'):
        """Draw the entire tree structure"""
        self.delete('all')
        
        if not tree.root:
            self.create_text(
                self.winfo_width() // 2, 
                self.winfo_height() // 2,
                text="Empty Tree",
                font=('Arial', 16, 'italic'),
                fill='gray'
            )
            return
        
        # Calculate tree dimensions
        width = self.winfo_width()
        height = self.winfo_height()
        
        # Draw tree recursively
        self._draw_node(tree.root, width // 2, 40, width // 4, tree_type)
        
    def _draw_node(self, node, x, y, h_offset, tree_type):
        """Recursively draw a node and its children"""
        if not node:
            return
        
        # Draw connections to children first (so they appear behind nodes)
        if node.left:
            child_x = x - h_offset
            child_y = y + self.level_height
            self.create_line(x, y, child_x, child_y, fill='gray', width=2)
            self._draw_node(node.left, child_x, child_y, h_offset // 2, tree_type)
        
        if node.right:
            child_x = x + h_offset
            child_y = y + self.level_height
            self.create_line(x, y, child_x, child_y, fill='gray', width=2)
            self._draw_node(node.right, child_x, child_y, h_offset // 2, tree_type)
        
        # Draw the node
        # Color based on score (heatmap)
        max_score = 100  # Adjust based on your data
        intensity = min(node.score / max_score, 1.0)
        
        if tree_type == 'treap':
            # Green shades for treap
            color = self._rgb_to_hex(46, int(204 * (0.5 + intensity * 0.5)), 113)
        else:
            # Red shades for bst
            color = self._rgb_to_hex(int(231 * (0.5 + intensity * 0.5)), 76, 60)
        
        self.create_oval(
            x - self.node_radius, y - self.node_radius,
            x + self.node_radius, y + self.node_radius,
            fill=color, outline='black', width=2
        )
        
        # Draw node text (show score)
        self.create_text(
            x, y - 5,
            text=f"{node.score}",
            font=('Arial', 10, 'bold'),
            fill='white'
        )
        
        # Draw post ID below
        self.create_text(
            x, y + 8,
            text=f"{node.postid[:6]}",
            font=('Arial', 7),
            fill='white'
        )
    
    def _rgb_to_hex(self, r, g, b):
        """Convert RGB to hex color"""
        return f'#{r:02x}{g:02x}{b:02x}'


class TreeTab(ttk.Frame):
    """Tab for managing a single tree (Treap or BST)"""
    
    def __init__(self, parent, tree_type='treap'):
        super().__init__(parent)
        self.tree_type = tree_type
        self.tree = Treap() if tree_type == 'treap' else BST()
        self.operation_count = 0
        
        self.setup_ui()
        
    def setup_ui(self):
        """Setup the UI components"""
        # Main container with two columns
        left_panel = ttk.Frame(self)
        left_panel.pack(side=tk.LEFT, fill=tk.BOTH, expand=False, padx=5, pady=5)
        
        right_panel = ttk.Frame(self)
        right_panel.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # === LEFT PANEL: Controls ===
        
        # Operations Frame
        ops_frame = ttk.LabelFrame(left_panel, text="Operations", padding=10)
        ops_frame.pack(fill=tk.X, pady=5)
        
        # Add Post
        ttk.Label(ops_frame, text="Post ID:").grid(row=0, column=0, sticky=tk.W, pady=2)
        self.postid_entry = ttk.Entry(ops_frame, width=15)
        self.postid_entry.grid(row=0, column=1, pady=2)
        
        ttk.Label(ops_frame, text="Timestamp:").grid(row=1, column=0, sticky=tk.W, pady=2)
        self.timestamp_entry = ttk.Entry(ops_frame, width=15)
        self.timestamp_entry.grid(row=1, column=1, pady=2)
        self.timestamp_entry.insert(0, str(int(time.time())))
        
        ttk.Label(ops_frame, text="Score:").grid(row=2, column=0, sticky=tk.W, pady=2)
        self.score_entry = ttk.Entry(ops_frame, width=15)
        self.score_entry.grid(row=2, column=1, pady=2)
        self.score_entry.insert(0, "0")
        
        ttk.Button(ops_frame, text="Add Post", command=self.add_post).grid(
            row=3, column=0, columnspan=2, pady=5, sticky=tk.EW
        )
        
        ttk.Separator(ops_frame, orient=tk.HORIZONTAL).grid(
            row=4, column=0, columnspan=2, sticky=tk.EW, pady=5
        )
        
        # Quick operations
        ttk.Label(ops_frame, text="Post ID (for ops):").grid(row=5, column=0, sticky=tk.W, pady=2)
        self.op_postid_entry = ttk.Entry(ops_frame, width=15)
        self.op_postid_entry.grid(row=5, column=1, pady=2)
        
        ttk.Button(ops_frame, text="Like Post", command=self.like_post).grid(
            row=6, column=0, columnspan=2, pady=2, sticky=tk.EW
        )
        ttk.Button(ops_frame, text="Delete Post", command=self.delete_post).grid(
            row=7, column=0, columnspan=2, pady=2, sticky=tk.EW
        )
        ttk.Button(ops_frame, text="Search Post", command=self.search_post).grid(
            row=8, column=0, columnspan=2, pady=2, sticky=tk.EW
        )
        
        # Bulk Import Frame
        import_frame = ttk.LabelFrame(left_panel, text="Bulk Import", padding=10)
        import_frame.pack(fill=tk.X, pady=5)
        
        ttk.Label(import_frame, text="Threads:").grid(row=0, column=0, sticky=tk.W, pady=2)
        self.threads_spinbox = ttk.Spinbox(import_frame, from_=1, to=16, width=13)
        self.threads_spinbox.set(4)
        self.threads_spinbox.grid(row=0, column=1, pady=2)
        
        ttk.Label(import_frame, text="Limit/file:").grid(row=1, column=0, sticky=tk.W, pady=2)
        self.limit_spinbox = ttk.Spinbox(import_frame, from_=0, to=100000, width=13)
        self.limit_spinbox.set(0)
        self.limit_spinbox.grid(row=1, column=1, pady=2)
        
        ttk.Button(import_frame, text="Import CSV", command=self.import_csv).grid(
            row=2, column=0, columnspan=2, pady=5, sticky=tk.EW
        )
        
        self.import_status = ttk.Label(import_frame, text="", foreground='green')
        self.import_status.grid(row=3, column=0, columnspan=2)
        
        # Query Frame
        query_frame = ttk.LabelFrame(left_panel, text="Queries", padding=10)
        query_frame.pack(fill=tk.X, pady=5)
        
        ttk.Button(query_frame, text="Get Most Popular", command=self.get_most_popular).pack(
            fill=tk.X, pady=2
        )
        
        ttk.Label(query_frame, text="Get Recent (k):").pack(anchor=tk.W, pady=(5,0))
        self.k_spinbox = ttk.Spinbox(query_frame, from_=1, to=100, width=13)
        self.k_spinbox.set(5)
        self.k_spinbox.pack(fill=tk.X)
        
        ttk.Button(query_frame, text="Get Most Recent", command=self.get_most_recent).pack(
            fill=tk.X, pady=2
        )
        
        # Metrics Frame
        metrics_frame = ttk.LabelFrame(left_panel, text="Metrics", padding=10)
        metrics_frame.pack(fill=tk.BOTH, expand=True, pady=5)
        
        self.metrics_text = scrolledtext.ScrolledText(
            metrics_frame, height=10, width=30, font=('Courier', 9)
        )
        self.metrics_text.pack(fill=tk.BOTH, expand=True)
        
        ttk.Button(metrics_frame, text="Refresh Metrics", command=self.update_metrics).pack(
            fill=tk.X, pady=(5,0)
        )
        
        # === RIGHT PANEL: Visualization and Log ===
        
        # Visualization Canvas
        viz_frame = ttk.LabelFrame(right_panel, text=f"{self.tree_type.upper()} Visualization")
        viz_frame.pack(fill=tk.BOTH, expand=True, pady=5)
        
        self.canvas = TreeVisualizer(viz_frame, width=800, height=400)
        self.canvas.pack(fill=tk.BOTH, expand=True)
        
        ttk.Button(viz_frame, text="Refresh Visualization", command=self.refresh_visualization).pack(
            pady=5
        )
        
        # Operation Log
        log_frame = ttk.LabelFrame(right_panel, text="Operation Log")
        log_frame.pack(fill=tk.BOTH, expand=True, pady=5)
        
        self.log_text = scrolledtext.ScrolledText(
            log_frame, height=10, font=('Courier', 9)
        )
        self.log_text.pack(fill=tk.BOTH, expand=True)
        
        # Clear log button
        ttk.Button(log_frame, text="Clear Log", command=self.clear_log).pack(pady=2)
        
        # Initial updates
        self.update_metrics()
        self.refresh_visualization()
    
    def log(self, message):
        """Add message to log with timestamp"""
        timestamp = time.strftime("%H:%M:%S")
        self.log_text.insert(tk.END, f"[{timestamp}] {message}\n")
        self.log_text.see(tk.END)
        self.operation_count += 1
    
    def clear_log(self):
        """Clear the operation log"""
        self.log_text.delete(1.0, tk.END)
    
    def add_post(self):
        """Add a new post to the tree"""
        try:
            postid = self.postid_entry.get().strip()
            timestamp = int(self.timestamp_entry.get().strip())
            score = int(self.score_entry.get().strip())
            
            if not postid:
                messagebox.showerror("Error", "Post ID cannot be empty")
                return
            
            start_time = time.perf_counter()
            result = self.tree.addPost(postid, timestamp, score)
            elapsed = (time.perf_counter() - start_time) * 1000
            
            if result:
                self.log(f"✓ Added post '{postid}' (Time: {elapsed:.4f}ms)")
                self.refresh_visualization()
                self.update_metrics()
                
                # Clear entries
                self.postid_entry.delete(0, tk.END)
                self.score_entry.delete(0, tk.END)
                self.score_entry.insert(0, "0")
            else:
                self.log(f"✗ Post '{postid}' already exists (Time: {elapsed:.4f}ms)")
                
        except ValueError as e:
            messagebox.showerror("Error", f"Invalid input: {e}")
    
    def like_post(self):
        """Like a post (increment score)"""
        postid = self.op_postid_entry.get().strip()
        if not postid:
            messagebox.showerror("Error", "Post ID cannot be empty")
            return
        
        start_time = time.perf_counter()
        result = self.tree.likePost(postid)
        elapsed = (time.perf_counter() - start_time) * 1000
        
        if result:
            self.log(f"✓ Liked post '{postid}' (Time: {elapsed:.4f}ms)")
            self.refresh_visualization()
            self.update_metrics()
        else:
            self.log(f"✗ Post '{postid}' not found (Time: {elapsed:.4f}ms)")
    
    def delete_post(self):
        """Delete a post from the tree"""
        postid = self.op_postid_entry.get().strip()
        if not postid:
            messagebox.showerror("Error", "Post ID cannot be empty")
            return
        
        start_time = time.perf_counter()
        result = self.tree.deletePost(postid)
        elapsed = (time.perf_counter() - start_time) * 1000
        
        if result:
            self.log(f"✓ Deleted post '{postid}' (Time: {elapsed:.4f}ms)")
            self.refresh_visualization()
            self.update_metrics()
        else:
            self.log(f"✗ Post '{postid}' not found (Time: {elapsed:.4f}ms)")
    
    def search_post(self):
        """Search for a post"""
        postid = self.op_postid_entry.get().strip()
        if not postid:
            messagebox.showerror("Error", "Post ID cannot be empty")
            return
        
        start_time = time.perf_counter()
        node = self.tree.id_map.get(postid)
        elapsed = (time.perf_counter() - start_time) * 1000
        
        if node:
            self.log(f"✓ Found '{postid}': ts={node.timestamp}, score={node.score} (Time: {elapsed:.4f}ms)")
        else:
            self.log(f"✗ Post '{postid}' not found (Time: {elapsed:.4f}ms)")
    
    def get_most_popular(self):
        """Get the most popular post"""
        start_time = time.perf_counter()
        result = self.tree.getMostPopular()
        elapsed = (time.perf_counter() - start_time) * 1000
        
        if result:
            postid, timestamp, score = result
            self.log(f"Most Popular: '{postid}' (score={score}, ts={timestamp}) (Time: {elapsed:.4f}ms)")
        else:
            self.log(f"No posts in tree (Time: {elapsed:.4f}ms)")
    
    def get_most_recent(self):
        """Get k most recent posts"""
        try:
            k = int(self.k_spinbox.get())
            start_time = time.perf_counter()
            results = self.tree.getMostRecent(k)
            elapsed = (time.perf_counter() - start_time) * 1000
            
            self.log(f"Top {k} Recent Posts (Time: {elapsed:.4f}ms):")
            for i, (postid, timestamp, score) in enumerate(results, 1):
                self.log(f"  {i}. '{postid}' (score={score}, ts={timestamp})")
                
        except ValueError:
            messagebox.showerror("Error", "Invalid k value")
    
    def import_csv(self):
        """Import posts from CSV file(s)"""
        filepaths = filedialog.askopenfilenames(
            title="Select CSV files",
            filetypes=[("CSV files", "*.csv"), ("All files", "*.*")]
        )
        
        if not filepaths:
            return
        
        try:
            threads = int(self.threads_spinbox.get())
            limit = int(self.limit_spinbox.get())
            limit = limit if limit > 0 else None
            
            self.import_status.config(text="Importing...", foreground='orange')
            self.update()
            
            # Import in background thread
            thread = threading.Thread(
                target=self._import_csv_worker,
                args=(filepaths, threads, limit)
            )
            thread.daemon = True
            thread.start()
            
        except ValueError as e:
            messagebox.showerror("Error", f"Invalid input: {e}")
    
    def _import_csv_worker(self, filepaths, threads, limit):
        """Background worker for CSV import"""
        start_time = time.time()
        total_added = 0
        
        for filepath in filepaths:
            try:
                with open(filepath, 'r', encoding='utf-8') as f:
                    reader = csv.DictReader(f)
                    count = 0
                    
                    for i, row in enumerate(reader):
                        if limit and i >= limit:
                            break
                        
                        try:
                            postid = row.get('id', row.get('postid', ''))
                            timestamp = int(row.get('created_utc', row.get('timestamp', 0)))
                            score = int(row.get('score', 0))
                            
                            if postid and timestamp:
                                if self.tree.addPost(postid, timestamp, score):
                                    count += 1
                                    total_added += 1
                        except (ValueError, KeyError):
                            continue
                    
                    self.log(f"Imported {count} posts from {Path(filepath).name}")
                    
            except Exception as e:
                self.log(f"Error importing {Path(filepath).name}: {e}")
        
        elapsed = time.time() - start_time
        
        # Update UI in main thread
        self.after(0, self._import_complete, total_added, elapsed)
    
    def _import_complete(self, total_added, elapsed):
        """Called when import is complete"""
        self.import_status.config(
            text=f"✓ Imported {total_added} posts in {elapsed:.2f}s",
            foreground='green'
        )
        self.log(f"✓ Bulk import complete: {total_added} posts in {elapsed:.2f}s")
        self.refresh_visualization()
        self.update_metrics()
    
    def update_metrics(self):
        """Update metrics display"""
        metrics = self.tree.getMetrics()
        
        self.metrics_text.delete(1.0, tk.END)
        self.metrics_text.insert(tk.END, f"{'='*30}\n")
        self.metrics_text.insert(tk.END, f"{self.tree_type.upper()} METRICS\n")
        self.metrics_text.insert(tk.END, f"{'='*30}\n\n")
        
        self.metrics_text.insert(tk.END, f"Size: {metrics['size']} nodes\n")
        self.metrics_text.insert(tk.END, f"Height: {metrics['height']}\n")
        
        if self.tree_type == 'treap':
            self.metrics_text.insert(tk.END, f"Rotations: {metrics['rotations']}\n")
        
        self.metrics_text.insert(tk.END, f"\nAvg Times (ms):\n")
        self.metrics_text.insert(tk.END, f"  Insert: {metrics['avg_insert_time_ms']:.6f}\n")
        self.metrics_text.insert(tk.END, f"  Delete: {metrics['avg_delete_time_ms']:.6f}\n")
        self.metrics_text.insert(tk.END, f"  Search: {metrics['avg_search_time_ms']:.6f}\n")
        
        self.metrics_text.insert(tk.END, f"\nTotal Ops: {metrics['total_operations']}\n")
        self.metrics_text.insert(tk.END, f"UI Ops: {self.operation_count}\n")
    
    def refresh_visualization(self):
        """Refresh the tree visualization"""
        self.canvas.draw_tree(self.tree, self.tree_type)


class TreeComparisonApp:
    """Main application window"""
    
    def __init__(self, root, show_gui=True):
        self.root = root
        self.show_gui = show_gui
        
        if not show_gui:
            return
        
        self.root.title("Treap vs BST Comparison Tool")
        self.root.geometry("1400x900")
        
        # Setup menu
        self.setup_menu()
        
        # Create notebook for tabs
        self.notebook = ttk.Notebook(root)
        self.notebook.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Create tabs
        self.treap_tab = TreeTab(self.notebook, tree_type='treap')
        self.bst_tab = TreeTab(self.notebook, tree_type='bst')
        
        self.notebook.add(self.treap_tab, text="Treap")
        self.notebook.add(self.bst_tab, text="BST")
        
        # Status bar
        self.status_bar = ttk.Label(
            root, text="Ready", relief=tk.SUNKEN, anchor=tk.W
        )
        self.status_bar.pack(side=tk.BOTTOM, fill=tk.X)
        
        self.update_status()
    
    def setup_menu(self):
        """Setup menu bar"""
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)
        
        # File menu
        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="File", menu=file_menu)
        file_menu.add_command(label="Exit", command=self.root.quit)
        
        # Help menu
        help_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Help", menu=help_menu)
        help_menu.add_command(label="About", command=self.show_about)
    
    def update_status(self):
        """Update status bar"""
        treap_size = self.treap_tab.tree.getSize()
        bst_size = self.bst_tab.tree.getSize()
        
        self.status_bar.config(
            text=f"Treap: {treap_size} nodes | BST: {bst_size} nodes | "
                 f"Ready"
        )
        
        self.root.after(1000, self.update_status)
    
    def show_about(self):
        """Show about dialog"""
        messagebox.showinfo(
            "About",
            "Treap vs BST Comparison Tool\n\n"
            "Compare performance between Treap and BST data structures.\n"
            "Visualize tree structure and track operation metrics.\n\n"
            "Features:\n"
            "- Real-time tree visualization\n"
            "- Bulk CSV import\n"
            "- Performance metrics\n"
            "- Operation logging"
        )


def main():
    """Main entry point"""
    import argparse
    
    parser = argparse.ArgumentParser(description='Tree Comparison Tool')
    parser.add_argument('--cli', action='store_true', 
                       help='Run in CLI mode (no visualization)')
    parser.add_argument('--gui', action='store_true', 
                       help='Run in GUI mode (with visualization)')
    
    args = parser.parse_args()
    
    # Default to GUI if neither specified
    if not args.cli and not args.gui:
        # Show initial choice dialog
        root = tk.Tk()
        root.withdraw()
        
        choice = messagebox.askyesno(
            "Mode Selection",
            "Do you want to run with visualization?\n\n"
            "Yes - GUI Mode (with tree visualization)\n"
            "No - CLI Mode (command line only)"
        )
        
        if choice:
            args.gui = True
        else:
            args.cli = True
        
        root.destroy()
    
    if args.cli:
        print("Running in CLI mode...")
        print("Use main.py for CLI operations")
        # You can add CLI functionality here
        return
    
    # GUI Mode
    root = tk.Tk()
    app = TreeComparisonApp(root, show_gui=True)
    root.mainloop()


if __name__ == "__main__":
    main()