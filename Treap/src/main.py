import argparse
import json
import time
import csv
import threading
from queue import Queue
from pathlib import Path
from typing import List, Dict
import matplotlib.pyplot as plt
import seaborn as sns
from treap import Treap
from bst import BST
import glob

try:
    from config import (
        DEFAULT_THREADS, DEFAULT_OUTPUT_DIR, PLOT_STYLE,
        FIGURE_DPI, TREAP_COLOR, BST_COLOR
    )
except ImportError:
    DEFAULT_THREADS = 4
    DEFAULT_OUTPUT_DIR = "results"
    PLOT_STYLE = "whitegrid"
    FIGURE_DPI = 300
    TREAP_COLOR = '#2ecc71'
    BST_COLOR = '#e74c3c'


class MultiCSVTreeBuilder:
    """Build trees from multiple CSV files in parallel"""
    
    def __init__(self, num_threads: int = 4):
        self.num_threads = num_threads
        self.treaps: List[Treap] = []
        self.bsts: List[BST] = []
        self.lock = threading.Lock()
        self.file_queue = Queue()
        
    def load_csv_file(self, filepath: str, limit: int = None) -> List[Dict]:
        """Load data from a single CSV file"""
        data = []
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                reader = csv.DictReader(f)
                for i, row in enumerate(reader):
                    if limit and i >= limit:
                        break
                    data.append(row)
            return data
        except Exception as e:
            print(f"Error loading {filepath}: {e}")
            return []
    
    def process_csv_file(self, filepath: str, tree_type: str, tree_list: List, limit: int = None):
        """Process a single CSV file and build a tree"""
        print(f"  Processing {Path(filepath).name} for {tree_type.upper()}...")
        
        # Load data from this CSV
        data = self.load_csv_file(filepath, limit)
        if not data:
            return
        
        # Create tree
        if tree_type == "treap":
            tree = Treap()
        else:
            tree = BST()
        
        # Add data to tree
        for entry in data:
            try:
                postid = entry.get('id', entry.get('postid', ''))
                timestamp = int(entry.get('created_utc', entry.get('timestamp', 0)))
                score = int(entry.get('score', 0))
                
                if postid and timestamp:
                    tree.addPost(postid, timestamp, score)
            except (ValueError, KeyError) as e:
                continue
        
        # Add to list thread-safely
        with self.lock:
            tree_list.append(tree)
        
        print(f"    ✓ {Path(filepath).name}: Added {tree.getSize()} nodes")
    
    def build_from_csv_files(self, csv_files: List[str], limit_per_file: int = None) -> tuple:
        """Build trees from multiple CSV files in parallel"""
        print(f"\n{'='*60}")
        print(f"Building trees from {len(csv_files)} CSV files")
        print(f"Using {self.num_threads} threads")
        print(f"{'='*60}\n")
        
        # Display files to process
        print("Files to process:")
        for i, f in enumerate(csv_files, 1):
            print(f"  {i}. {Path(f).name}")
        print()
        
        # Build Treaps from all CSV files in parallel
        print("Building Treaps from all CSV files...")
        treap_threads = []
        for csv_file in csv_files:
            thread = threading.Thread(
                target=self.process_csv_file,
                args=(csv_file, "treap", self.treaps, limit_per_file)
            )
            thread.start()
            treap_threads.append(thread)
            
            # Limit concurrent threads
            if len(treap_threads) >= self.num_threads:
                for t in treap_threads:
                    t.join()
                treap_threads = []
        
        # Wait for remaining threads
        for thread in treap_threads:
            thread.join()
        
        print(f"✓ Created {len(self.treaps)} Treaps from CSV files\n")
        
        # Build BSTs from all CSV files in parallel
        print("Building BSTs from all CSV files...")
        bst_threads = []
        for csv_file in csv_files:
            thread = threading.Thread(
                target=self.process_csv_file,
                args=(csv_file, "bst", self.bsts, limit_per_file)
            )
            thread.start()
            bst_threads.append(thread)
            
            # Limit concurrent threads
            if len(bst_threads) >= self.num_threads:
                for t in bst_threads:
                    t.join()
                bst_threads = []
        
        # Wait for remaining threads
        for thread in bst_threads:
            thread.join()
        
        print(f"✓ Created {len(self.bsts)} BSTs from CSV files\n")
        
        # Merge all Treaps
        print(f"Merging {len(self.treaps)} Treaps...")
        start_time = time.time()
        merged_treap = self.treaps[0] if self.treaps else Treap()
        for i in range(1, len(self.treaps)):
            print(f"  Merging Treap {i+1}/{len(self.treaps)}...")
            merged_treap = merged_treap.union(self.treaps[i])
        treap_merge_time = time.time() - start_time
        print(f"✓ Treap merge completed in {treap_merge_time:.2f}s")
        print(f"  Final Treap size: {merged_treap.getSize()} nodes\n")
        
        # Merge all BSTs (manual merge)
        print(f"Merging {len(self.bsts)} BSTs...")
        start_time = time.time()
        merged_bst = BST()
        total_nodes = 0
        for i, bst in enumerate(self.bsts, 1):
            print(f"  Merging BST {i}/{len(self.bsts)}...")
            for postid, node in bst.id_map.items():
                if postid not in merged_bst.id_map:  # Avoid duplicates
                    merged_bst.addPost(postid, node.timestamp, node.score)
                    total_nodes += 1
        bst_merge_time = time.time() - start_time
        print(f"✓ BST merge completed in {bst_merge_time:.2f}s")
        print(f"  Final BST size: {merged_bst.getSize()} nodes\n")
        
        return merged_treap, merged_bst


def find_csv_files(patterns: List[str]) -> List[str]:
    """Find all CSV files matching the given patterns"""
    csv_files = []
    for pattern in patterns:
        if '*' in pattern or '?' in pattern:
            # Glob pattern
            matched = glob.glob(pattern)
            csv_files.extend(matched)
        else:
            # Direct file path
            if Path(pattern).exists():
                csv_files.append(pattern)
            else:
                print(f"Warning: File not found: {pattern}")
    
    # Remove duplicates and sort
    csv_files = sorted(list(set(csv_files)))
    return csv_files


def calculate_balance_factor(tree) -> float:
    """Calculate balance factor as height / log2(n+1)"""
    import math
    size = tree.getSize()
    height = tree.getHeight()
    
    if size == 0:
        return 0.0
    
    optimal_height = math.log2(size + 1)
    return height / optimal_height if optimal_height > 0 else 0.0


def save_metrics(tree, filename: str, tree_type: str):
    """Save tree metrics to JSON file"""
    metrics = tree.getMetrics()
    metrics['tree_balance_factor'] = calculate_balance_factor(tree)
    
    output = {
        "tree_type": tree_type,
        "summary": metrics,
        "snapshots": []
    }
    
    with open(filename, 'w') as f:
        json.dump(output, f, indent=2)
    
    print(f"Metrics saved to {filename}")


def compare_and_visualize(treap_file: str, bst_file: str, output_dir: str = "results"):
    """Compare metrics and create visualizations"""
    Path(output_dir).mkdir(exist_ok=True)
    
    # Load metrics
    with open(treap_file, 'r') as f:
        treap_data = json.load(f)
    with open(bst_file, 'r') as f:
        bst_data = json.load(f)
    
    treap_metrics = treap_data['summary']
    bst_metrics = bst_data['summary']
    
    # Set style
    sns.set_style(PLOT_STYLE)
    plt.rcParams['figure.figsize'] = (15, 10)
    
    # Create comparison plots
    fig, axes = plt.subplots(2, 3, figsize=(18, 12))
    fig.suptitle('Treap vs BST Performance Comparison', fontsize=16, fontweight='bold')
    
    # 1. Height Comparison
    ax = axes[0, 0]
    heights = [treap_metrics['height'], bst_metrics['height']]
    bars = ax.bar(['Treap', 'BST'], heights, color=[TREAP_COLOR, BST_COLOR])
    ax.set_ylabel('Height')
    ax.set_title('Tree Height (Lower is Better)')
    for bar in bars:
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height,
                f'{int(height)}', ha='center', va='bottom')
    
    # 2. Size Comparison
    ax = axes[0, 1]
    sizes = [treap_metrics['size'], bst_metrics['size']]
    bars = ax.bar(['Treap', 'BST'], sizes, color=[TREAP_COLOR, BST_COLOR])
    ax.set_ylabel('Number of Nodes')
    ax.set_title('Tree Size')
    for bar in bars:
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height,
                f'{int(height)}', ha='center', va='bottom')
    
    # 3. Balance Factor
    ax = axes[0, 2]
    balance_factors = [treap_metrics['tree_balance_factor'], bst_metrics['tree_balance_factor']]
    bars = ax.bar(['Treap', 'BST'], balance_factors, color=[TREAP_COLOR, BST_COLOR])
    ax.set_ylabel('Balance Factor')
    ax.set_title('Tree Balance Factor (Closer to 1.0 is Better)')
    ax.axhline(y=1.0, color='gray', linestyle='--', alpha=0.5)
    for bar in bars:
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.2f}', ha='center', va='bottom')
    
    # 4. Average Insert Time
    ax = axes[1, 0]
    insert_times = [treap_metrics['avg_insert_time_ms'], bst_metrics['avg_insert_time_ms']]
    bars = ax.bar(['Treap', 'BST'], insert_times, color=[TREAP_COLOR, BST_COLOR])
    ax.set_ylabel('Time (ms)')
    ax.set_title('Average Insert Time (Lower is Better)')
    for bar in bars:
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.4f}', ha='center', va='bottom')
    
    # 5. Rotations (Treap only)
    ax = axes[1, 1]
    rotations = [treap_metrics['rotations'], 0]
    bars = ax.bar(['Treap', 'BST'], rotations, color=[TREAP_COLOR, BST_COLOR])
    ax.set_ylabel('Number of Rotations')
    ax.set_title('Total Rotations')
    for bar in bars:
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height,
                f'{int(height)}', ha='center', va='bottom')
    
    # 6. Operations Summary
    ax = axes[1, 2]
    operations = [treap_metrics['total_operations'], bst_metrics['total_operations']]
    bars = ax.bar(['Treap', 'BST'], operations, color=[TREAP_COLOR, BST_COLOR])
    ax.set_ylabel('Number of Operations')
    ax.set_title('Total Operations')
    for bar in bars:
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height,
                f'{int(height)}', ha='center', va='bottom')
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/comparison.png', dpi=FIGURE_DPI, bbox_inches='tight')
    print(f"Comparison plot saved to {output_dir}/comparison.png")
    
    # Print summary
    print("\n" + "="*60)
    print("PERFORMANCE COMPARISON SUMMARY")
    print("="*60)
    print(f"\n{'Metric':<30} {'Treap':<15} {'BST':<15} {'Winner'}")
    print("-"*60)
    
    def compare_metric(name, treap_val, bst_val, lower_better=True):
        if lower_better:
            winner = "Treap" if treap_val < bst_val else "BST" if bst_val < treap_val else "Tie"
        else:
            winner = "Treap" if treap_val > bst_val else "BST" if bst_val > treap_val else "Tie"
        print(f"{name:<30} {treap_val:<15.4f} {bst_val:<15.4f} {winner}")
    
    compare_metric("Height", treap_metrics['height'], bst_metrics['height'], True)
    compare_metric("Balance Factor", treap_metrics['tree_balance_factor'], 
                   bst_metrics['tree_balance_factor'], True)
    compare_metric("Avg Insert Time (ms)", treap_metrics['avg_insert_time_ms'], 
                   bst_metrics['avg_insert_time_ms'], True)
    print(f"{'Rotations':<30} {treap_metrics['rotations']:<15} {'N/A':<15} {'N/A'}")
    print("="*60)


def main():
    parser = argparse.ArgumentParser(
        description='Compare Treap vs BST performance with multiple CSV files',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Single file
  python main.py data.csv

  # Multiple files
  python main.py file1.csv file2.csv file3.csv

  # Using glob patterns
  python main.py data/*.csv
  python main.py data/part*.csv
  
  # Multiple patterns
  python main.py data1.csv "data/chunk*.csv" file3.csv
  
  # With options
  python main.py *.csv --limit-per-file 1000 --threads 8 --visualize
        """
    )
    
    parser.add_argument('input_files', nargs='+', 
                       help='Input CSV file(s) or glob patterns')
    parser.add_argument('--limit-per-file', type=int, 
                       help='Limit number of entries per CSV file')
    parser.add_argument('--threads', type=int, default=DEFAULT_THREADS, 
                       help=f'Number of threads (default: {DEFAULT_THREADS})')
    parser.add_argument('--visualize', action='store_true', 
                       help='Generate comparison visualizations')
    parser.add_argument('--output-dir', default=DEFAULT_OUTPUT_DIR, 
                       help=f'Output directory (default: {DEFAULT_OUTPUT_DIR})')
    
    args = parser.parse_args()
    
    # Find all CSV files
    csv_files = find_csv_files(args.input_files)
    
    if not csv_files:
        print("Error: No CSV files found!")
        print(f"Patterns provided: {args.input_files}")
        return
    
    print(f"\nFound {len(csv_files)} CSV file(s)")
    
    # Build trees from all CSV files
    builder = MultiCSVTreeBuilder(num_threads=args.threads)
    treap, bst = builder.build_from_csv_files(csv_files, args.limit_per_file)
    
    # Save metrics
    print("Saving metrics...")
    save_metrics(treap, 'treap_metrics.json', 'Treap')
    save_metrics(bst, 'bst_metrics.json', 'BST')
    print()
    
    # Compare and visualize
    if args.visualize:
        print("Generating comparison visualizations...")
        compare_and_visualize('treap_metrics.json', 'bst_metrics.json', args.output_dir)
    
    print("\n" + "="*60)
    print("✓ Processing complete!")
    print("="*60)
    print(f"Total files processed: {len(csv_files)}")
    print(f"Final Treap size: {treap.getSize()} nodes")
    print(f"Final BST size: {bst.getSize()} nodes")
    print("="*60)


if __name__ == "__main__":
    main()