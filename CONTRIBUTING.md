# Contributing to RFC

Thank you for your interest in contributing to the Remote Feature Control (RFC) project!

---

## Getting Started

1. **Fork** the repository on GitHub
2. **Clone** your fork locally
3. **Create** a feature branch from `develop`:
   ```bash
   git checkout -b feature/JIRA-XXXX-description develop
   ```
4. **Build** and verify your changes:
   ```bash
   autoreconf -i
   ./configure --enable-gtestapp
   make && make check
   ```
5. **Commit** with a clear message referencing the JIRA ticket
6. **Push** your branch and open a Pull Request against `develop`

---

## Branch Naming

| Type | Pattern | Example |
|------|---------|---------|
| Feature | `feature/JIRA-XXXX-short-desc` | `feature/RDKC-15902-camera-support` |
| Bugfix | `bugfix/JIRA-XXXX-short-desc` | `bugfix/DELIA-69554-cert-retry` |
| Release | `release/X.Y.Z` | `release/1.2.3` |

---

## Coding Standards

- **C++ Standard:** C++11 minimum
- **Platform guards:** Use `#ifdef RDKC`, `#ifdef RDKB_SUPPORT`, or `#ifndef` blocks for platform-specific code
- **Comments:** Use Doxygen-style documentation (`/** @brief ... */`, `@param`, `@return`)
- **Logging:** Use `RDK_LOG_*` macros with appropriate log levels
- **Security:** Use `v_secure_system()` / `v_secure_popen()` instead of raw `system()` / `popen()`

---

## Pull Request Checklist

- [ ] Code compiles for all target platforms (STB, RDKB, RDKC)
- [ ] Unit tests pass (`make check`)
- [ ] New functionality includes corresponding tests
- [ ] No Coverity or static analysis warnings introduced
- [ ] Platform-specific code is properly guarded with `#ifdef`
- [ ] Documentation updated if architecture changes were made

---

## Testing

```bash
# Unit tests (L1)
./run_ut.sh

# Integration tests (L2)
./run_l2.sh

# Reboot trigger tests
./run_l2_reboot_trigger.sh
```

---

## Contributor License Agreement

Before RDK accepts your code into the project you must sign the [RDK Contributor License Agreement (CLA)](https://wiki.rdkcentral.com/display/DOC/Contributor+License+Agreement).

---

## Repository Documentation

For architecture details, diagrams, and data flow documentation, see the [documentation/](documentation/) folder:

- [Architecture](documentation/architecture.md) — Component architecture, class hierarchy, build system
- [Sequence Diagrams](documentation/sequence-diagrams.md) — End-to-end execution flows
- [Data Processing Flow](documentation/data-processing-flow.md) — Parameter lifecycle and storage strategies
