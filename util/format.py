#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later

import subprocess
import sys
import argparse
from pathlib import Path

def find_clang_format():
    """Find clang-format in PATH."""
    try:
        result = subprocess.run(['which', 'clang-format'], 
                              capture_output=True, text=True, check=True)
        return result.stdout.strip()
    except subprocess.CalledProcessError:
        return None

def get_source_files(directories):
    """Get all C++ source files from directories."""
    extensions = {'.cpp', '.h', '.hpp', '.c', '.cc', '.cxx', '.mm'}
    files = []
    
    for directory in directories:
        path = Path(directory)
        if not path.exists():
            print(f"Warning: Directory not found: {directory}", file=sys.stderr)
            continue
        
        for ext in extensions:
            files.extend(path.rglob(f'*{ext}'))
    
    return files

def main():
    parser = argparse.ArgumentParser(description='Format C++ code with clang-format')
    parser.add_argument('--check', action='store_true', 
                       help='Check if formatting is needed without modifying files')
    args = parser.parse_args()
    
    # Find clang-format
    clang_format = find_clang_format()
    if not clang_format:
        print("Error: clang-format not found. Please install clang-format.", 
              file=sys.stderr)
        sys.exit(1)
    
    print(f"Using clang-format: {clang_format}")
    
    # Get source directories
    script_dir = Path(__file__).parent.parent
    directories = [
        script_dir / 'include',
        script_dir / 'src'
    ]
    
    # Get all source files
    files = get_source_files(directories)
    if not files:
        print("No source files found.")
        sys.exit(0)
    
    print(f"Found {len(files)} source files")
    
    # Run clang-format
    cmd = [clang_format]
    if args.check:
        cmd.append('--output-replacements-xml')
    else:
        cmd.append('-i')
    
    cmd.extend(str(f) for f in files)
    
    try:
        if args.check:
            result = subprocess.run(cmd, capture_output=True, text=True)
            if result.stdout and '<?xml' in result.stdout:
                print("Code base has not been formatted")
                sys.exit(1)
            else:
                print("All files are properly formatted")
                sys.exit(0)
        else:
            subprocess.run(cmd, check=True)
            print(f"Formatted {len(files)} files")
            sys.exit(0)
    except subprocess.CalledProcessError as e:
        print(f"Error running clang-format: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == '__main__':
    main()
