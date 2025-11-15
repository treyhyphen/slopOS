#!/bin/bash
# slopOS Version Management Script

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

show_help() {
    cat << EOF
slopOS Version Management

Usage: $0 [command] [version]

Commands:
    current              Show current version
    set <version>        Set specific version (e.g., 1.0.0 or v1.0.0)
    bump patch          Bump patch version (0.0.X)
    bump minor          Bump minor version (0.X.0)
    bump major          Bump major version (X.0.0)
    list                List all version tags
    help                Show this help message

Examples:
    $0 current                  # Show: v0.1.0
    $0 set 1.0.0               # Create tag: v1.0.0
    $0 set v2.3.4              # Create tag: v2.3.4
    $0 bump major              # v0.1.0 → v1.0.0
    $0 bump minor              # v0.1.0 → v0.2.0
    $0 bump patch              # v0.1.0 → v0.1.1

Note: This will create and push tags to the remote repository.
      Tags will trigger GitHub Actions to build and release.

EOF
}

get_latest_tag() {
    git fetch --tags 2>/dev/null || true
    LATEST=$(git describe --tags --abbrev=0 2>/dev/null || echo "v0.1.0")
    echo "$LATEST"
}

parse_version() {
    local version=$1
    # Remove 'v' prefix if present
    version=${version#v}
    
    IFS='.' read -ra PARTS <<< "$version"
    MAJOR=${PARTS[0]:-0}
    MINOR=${PARTS[1]:-0}
    PATCH=${PARTS[2]:-0}
    
    echo "$MAJOR $MINOR $PATCH"
}

create_tag() {
    local tag=$1
    local message=$2
    
    # Ensure tag starts with 'v'
    if [[ ! $tag == v* ]]; then
        tag="v$tag"
    fi
    
    # Check if tag already exists
    if git rev-parse "$tag" >/dev/null 2>&1; then
        echo "❌ Error: Tag $tag already exists!"
        exit 1
    fi
    
    echo "Creating tag: $tag"
    git tag -a "$tag" -m "${message:-Release $tag}"
    
    echo "Pushing tag to remote..."
    git push origin "$tag"
    
    echo "✅ Tag $tag created and pushed!"
    echo "   GitHub Actions will now build and create a release."
    echo "   View at: https://github.com/treyhyphen/slopOS/releases"
}

case "${1:-help}" in
    current)
        CURRENT=$(get_latest_tag)
        echo "Current version: $CURRENT"
        ;;
    
    set)
        if [ -z "$2" ]; then
            echo "❌ Error: Version required"
            echo "Usage: $0 set <version>"
            exit 1
        fi
        
        VERSION=$2
        create_tag "$VERSION" "Release $VERSION"
        ;;
    
    bump)
        BUMP_TYPE=${2:-patch}
        
        if [[ ! "$BUMP_TYPE" =~ ^(major|minor|patch)$ ]]; then
            echo "❌ Error: Invalid bump type. Use: major, minor, or patch"
            exit 1
        fi
        
        LATEST=$(get_latest_tag)
        echo "Current version: $LATEST"
        
        read MAJOR MINOR PATCH <<< $(parse_version "$LATEST")
        
        case "$BUMP_TYPE" in
            major)
                MAJOR=$((MAJOR + 1))
                MINOR=0
                PATCH=0
                ;;
            minor)
                MINOR=$((MINOR + 1))
                PATCH=0
                ;;
            patch)
                PATCH=$((PATCH + 1))
                ;;
        esac
        
        NEW_VERSION="v$MAJOR.$MINOR.$PATCH"
        echo "New version: $NEW_VERSION ($BUMP_TYPE bump)"
        
        read -p "Create and push this tag? [y/N] " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            create_tag "$NEW_VERSION" "Release $NEW_VERSION ($BUMP_TYPE bump)"
        else
            echo "Cancelled."
        fi
        ;;
    
    list)
        echo "All version tags:"
        git fetch --tags 2>/dev/null || true
        git tag -l "v*" | sort -V
        ;;
    
    help|--help|-h)
        show_help
        ;;
    
    *)
        echo "❌ Unknown command: $1"
        echo ""
        show_help
        exit 1
        ;;
esac
