# GitHub Actions Workflows

This directory contains automated CI/CD workflows for slopOS.

## Workflows

### 1. Build and Release (`build-and-release.yml`)

**Triggers:**
- Push to `main` or `master` branch
- Push of tags matching `v*`
- Pull requests to `main` or `master`
- Manual workflow dispatch

**What it does:**
- Installs build dependencies
- Compiles the slopOS simulator (`slopos_sim`)
- Uploads simulator as an artifact (30-day retention)
- On tagged releases: Creates a GitHub release with the simulator binary

**Outputs:**
- Artifact: `slopos-simulator` (slopos_sim binary)
- Release: Automatic on version tags (e.g., `v1.0.0`)

---

### 2. Build Bootable ISO (`build-iso.yml`)

**Triggers:**
- Push to `main` or `master` branch (only when kernel/boot files change)
- Manual workflow dispatch

**What it does:**
- Checks for kernel and boot directories
- Compiles the bootable kernel
- Creates a bootable ISO image
- Tests boot in QEMU (headless)
- Uploads ISO as an artifact (90-day retention)
- On tagged releases: Creates a GitHub release with ISO and kernel binary

**Outputs:**
- Artifact: `slopos-iso-{version}` (contains slopos.iso and slopos.bin)
- Release: Automatic on version tags (includes bootable ISO)

**Note:** This workflow only runs if kernel source files exist.

---

### 3. Auto-tag Release (`auto-tag.yml`)

**Triggers:**
- Push to `main` or `master` branch (only when source files change)

**What it does:**
- Automatically increments the patch version
- Creates and pushes a new version tag
- Example: `v0.1.0` → `v0.1.1` → `v0.1.2`

**Tag Format:** `vMAJOR.MINOR.PATCH` (e.g., `v1.2.3`)

**Note:** You can manually create tags for major/minor version bumps:
```bash
git tag -a v1.0.0 -m "Release v1.0.0"
git push origin v1.0.0
```

---

## How to Use

### Automatic Releases

1. **Commit and push changes** to the main branch:
   ```bash
   git add .
   git commit -m "Add new feature"
   git push origin main
   ```

2. **Auto-tag workflow** creates a new version tag automatically

3. **Build workflows** trigger on the new tag and create releases

### Manual Releases

1. **Create a version tag:**
   ```bash
   git tag -a v1.0.0 -m "Release version 1.0.0"
   git push origin v1.0.0
   ```

2. **GitHub Actions** automatically:
   - Builds the simulator
   - Creates the ISO (if kernel exists)
   - Creates a GitHub release
   - Uploads artifacts

### Manual Workflow Trigger

You can manually trigger workflows from the GitHub Actions tab:
1. Go to **Actions** tab
2. Select a workflow
3. Click **Run workflow**
4. Choose branch and click **Run workflow**

---

## Artifacts

### Simulator Artifact
- **Name:** `slopos-simulator`
- **Contents:** `slopos_sim` executable
- **Retention:** 30 days
- **How to use:**
  ```bash
  chmod +x slopos_sim
  ./slopos_sim
  ```

### ISO Artifact
- **Name:** `slopos-iso-{version}`
- **Contents:** `slopos.iso` and `slopos.bin`
- **Retention:** 90 days
- **How to use:**
  ```bash
  # QEMU
  qemu-system-i386 -cdrom slopos.iso -m 32M
  
  # Or mount in VirtualBox/VMware
  ```

---

## Requirements

The workflows automatically install these dependencies:
- build-essential
- gcc-multilib / g++-multilib
- nasm
- grub-pc-bin / grub-common
- xorriso / mtools
- qemu-system-x86

---

## Customization

### Change Version Increment Strategy

Edit `auto-tag.yml` to modify version bumping logic:
- Current: Auto-increments patch (0.0.X)
- Options: Increment minor (0.X.0) or major (X.0.0)

### Disable Auto-tagging

Remove or disable the `auto-tag.yml` workflow and manually create tags:
```bash
git tag -a v1.0.0 -m "Release message"
git push origin v1.0.0
```

### Add Additional Tests

Add test steps in `build-and-release.yml`:
```yaml
- name: Run tests
  run: |
    ./run_tests.sh
```

---

## Troubleshooting

### ISO Not Building
- Ensure `kernel/`, `boot/`, and `Makefile` exist
- Check workflow logs in GitHub Actions tab

### Release Not Created
- Verify tag format matches `v*` (e.g., `v1.0.0`)
- Check repository permissions for `GITHUB_TOKEN`

### Artifacts Not Available
- Check retention period (30/90 days)
- Verify workflow completed successfully

---

## Default Login Credentials

All releases include these default credentials:
- **Username:** `admin`
- **Password:** `slopOS123`
