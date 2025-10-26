#!/bin/bash

# Package script for Production Line Simulator
# Creates a distributable package with all necessary files

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
PACKAGE_DIR="$PROJECT_ROOT/package"
VERSION="1.0.0"
PACKAGE_NAME="ProductionLineSimulator-$VERSION"

echo "=== Production Line Simulator Package Script ==="
echo "Project root: $PROJECT_ROOT"
echo "Package name: $PACKAGE_NAME"

# Ensure project is built
if [ ! -f "$BUILD_DIR/ProductionLineSimulator" ]; then
    echo "Error: Project not built. Run ./scripts/run.sh first."
    exit 1
fi

# Clean and create package directory
echo "Preparing package directory..."
rm -rf "$PACKAGE_DIR"
mkdir -p "$PACKAGE_DIR/$PACKAGE_NAME"

cd "$PACKAGE_DIR/$PACKAGE_NAME"

# Copy executable
echo "Copying executable..."
cp "$BUILD_DIR/ProductionLineSimulator" .

# Copy documentation
echo "Copying documentation..."
cp -r "$PROJECT_ROOT/docs" .

# Copy configuration files
echo "Copying configuration files..."
mkdir -p config
if [ -f "$PROJECT_ROOT/config/default.json" ]; then
    cp "$PROJECT_ROOT/config/default.json" config/
fi

# Copy source code (for academic submission)
echo "Copying source code..."
cp -r "$PROJECT_ROOT/src" .
cp "$PROJECT_ROOT/CMakeLists.txt" .

# Copy build scripts
echo "Copying scripts..."
cp -r "$PROJECT_ROOT/scripts" .

# Copy resources if they exist
if [ -d "$PROJECT_ROOT/resources" ]; then
    echo "Copying resources..."
    cp -r "$PROJECT_ROOT/resources" .
fi

# Create README for package
echo "Creating package README..."
cat > README_PACKAGE.txt << 'EOF'
Production Line Simulator - Distribution Package
===============================================

This package contains the complete Production Line Simulator application
for IF-4001 Operating Systems (Group 21).

Contents:
- ProductionLineSimulator: Main executable
- docs/: Complete documentation including user guide
- src/: Source code files
- scripts/: Build and utility scripts
- CMakeLists.txt: Build configuration

Requirements:
- Ubuntu Desktop 20.04+ 
- Qt 6.2+ (sudo apt install qt6-base-dev qt6-charts-dev)
- CMake 3.22+ (sudo apt install cmake)

Quick Start:
1. Extract this package
2. Install dependencies (see docs/README.md)
3. Run: ./ProductionLineSimulator
   OR
   Build from source: ./scripts/run.sh

For detailed installation and usage instructions, see docs/README.md

Academic Information:
- Course: IF-4001 Operating Systems
- Group: 21
- Delivery: November 17, 2025
- Platform: Ubuntu Desktop (Linux)
- Tech Stack: C++20, Qt 6, CMake

EOF

# Create simple install script
echo "Creating install script..."
cat > install.sh << 'EOF'
#!/bin/bash

echo "=== Production Line Simulator Installation ==="

# Check for Qt6
if ! dpkg -l | grep -q qt6-base-dev; then
    echo "Installing Qt 6 dependencies..."
    sudo apt update
    sudo apt install -y qt6-base-dev qt6-charts-dev qt6-tools-dev
fi

# Make executable runnable
chmod +x ProductionLineSimulator
chmod +x scripts/*.sh

# Create desktop entry (optional)
DESKTOP_FILE="$HOME/.local/share/applications/production-line-simulator.desktop"
mkdir -p "$(dirname "$DESKTOP_FILE")"

cat > "$DESKTOP_FILE" << DESKTOP_EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=Production Line Simulator
Comment=Automated Home-Appliance Production Line Simulator
Icon=applications-engineering
Exec=$(pwd)/ProductionLineSimulator
Path=$(pwd)
Terminal=false
Categories=Education;Engineering;
DESKTOP_EOF

echo "Installation complete!"
echo "Run with: ./ProductionLineSimulator"
echo "Or use desktop shortcut in applications menu"

EOF

chmod +x install.sh

# Go back to package directory
cd "$PACKAGE_DIR"

# Create tarball
echo "Creating tarball..."
tar -czf "$PACKAGE_NAME.tar.gz" "$PACKAGE_NAME"

# Create zip file
echo "Creating zip file..."
zip -r "$PACKAGE_NAME.zip" "$PACKAGE_NAME" > /dev/null

# Display package info
echo ""
echo "=== Package Created Successfully ==="
echo "Location: $PACKAGE_DIR"
echo "Files created:"
echo "  - $PACKAGE_NAME.tar.gz"
echo "  - $PACKAGE_NAME.zip"
echo "  - $PACKAGE_NAME/ (directory)"
echo ""
echo "Package contents:"
ls -la "$PACKAGE_NAME"
echo ""

# Calculate sizes
TARBALL_SIZE=$(du -h "$PACKAGE_NAME.tar.gz" | cut -f1)
ZIP_SIZE=$(du -h "$PACKAGE_NAME.zip" | cut -f1)
DIR_SIZE=$(du -sh "$PACKAGE_NAME" | cut -f1)

echo "Package sizes:"
echo "  - Tarball: $TARBALL_SIZE"
echo "  - ZIP file: $ZIP_SIZE"
echo "  - Directory: $DIR_SIZE"
echo ""
echo "Ready for distribution!"
