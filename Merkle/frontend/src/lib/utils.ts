import { type ClassValue, clsx } from 'clsx';
import { twMerge } from 'tailwind-merge';

/**
 * Merge Tailwind CSS classes with proper precedence
 */
export function cn(...inputs: ClassValue[]) {
    return twMerge(clsx(inputs));
}

/**
 * Format hash for display (truncate middle)
 */
export function formatHash(hash: string, length: number = 16): string {
    if (hash.length <= length) return hash;
    const half = Math.floor(length / 2);
    return `${hash.slice(0, half)}...${hash.slice(-half)}`;
}

/**
 * Format number with commas
 */
export function formatNumber(num: number): string {
    return num.toLocaleString();
}

/**
 * Format bytes to human-readable size
 */
export function formatBytes(bytes: number): string {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return `${(bytes / Math.pow(k, i)).toFixed(2)} ${sizes[i]}`;
}

/**
 * Format milliseconds to human-readable time
 */
export function formatTime(ms: number): string {
    if (ms < 1000) return `${ms.toFixed(2)} ms`;
    if (ms < 60000) return `${(ms / 1000).toFixed(2)} s`;
    return `${(ms / 60000).toFixed(2)} min`;
}

/**
 * Copy text to clipboard
 */
export async function copyToClipboard(text: string): Promise<boolean> {
    try {
        await navigator.clipboard.writeText(text);
        return true;
    } catch (err) {
        console.error('Failed to copy:', err);
        return false;
    }
}

/**
 * Download data as JSON file
 */
export function downloadJSON(data: any, filename: string): void {
    const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = filename;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
}

/**
 * Get color for node type - Black and Neon Green theme
 */
export function getNodeColor(type: 'root' | 'internal' | 'leaf' | 'proof' | 'modified' | 'added' | 'deleted'): string {
    const colors = {
        root: '#00FF00',      // Bright neon green for root
        internal: '#FFD700',  // Gold for internal nodes
        leaf: '#00BFFF',      // Deep sky blue for leaves
        proof: '#FF00FF',     // Magenta for proof path
        modified: '#FFFF00',  // Yellow for modified nodes
        added: '#00FF88',     // Aquamarine for added
        deleted: '#FF0000',   // Red for deleted
    };
    return colors[type];
}

/**
 * Calculate tree depth from node count
 */
export function calculateTreeDepth(nodeCount: number): number {
    return Math.ceil(Math.log(nodeCount) / Math.log(4));
}
