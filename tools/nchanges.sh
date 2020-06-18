#!/bin/bash
git status --porcelain --untracked-files=no | wc -l
