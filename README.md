# RFC - Remote Feature Control

Remote Feature Control (RFC) is a system for RDK (Reference Design Kit) devices that allows enabling/disabling features remotely without requiring code changes or firmware updates.

## What is RFC?

RFC allows operators and developers to:
- Enable or disable features on devices remotely
- Roll out features to specific device groups
- Configure feature parameters without firmware updates
- Test features in production with easy rollback capability

## How It Works

1. **RFCManager** service runs on the device and periodically connects to an xconf server
2. The xconf server provides feature configurations based on device properties (model, firmware version, partner ID, etc.)
3. Configurations are downloaded and stored locally as `.ini` files in `/opt/secure/RFC/`
4. Applications query feature status using shell scripts or APIs
5. Features can be enabled/disabled without rebooting (if marked as "effectiveImmediate")

## Key Components

- **rfcMgr/** - C++ RFCManager service that fetches configurations
- **getRFC.sh** - Shell script to read RFC configuration files
- **isFeatureEnabled.sh** - Shell script to check if a feature is enabled
- **rfcapi/** - API for querying RFC parameters programmatically
- **tr181api/** - TR-181 interface for RFC parameters

## Quick Start

### Check if a Feature is Enabled

```bash
/lib/rdk/isFeatureEnabled.sh FEATURE_NAME
```

Returns:
- `1` if feature is enabled
- `0` if feature is disabled or not configured
- `-1` on error

### Get RFC Configuration

```bash
source /lib/rdk/getRFC.sh FEATURE_NAME
echo $RFC_ENABLE_FEATURE_NAME
```

### Check All RFC Variables

```bash
cat /opt/secure/RFC/rfcVariable.ini
```

## Understanding Logs

**Quick Answer**: If you see "NOT IN RFC!" in the logs, it's **NOT an error**. It simply means that feature is not configured on the xconf server, and the system defaults to disabled.

For detailed log interpretation, see: **[RFC Log Interpretation Guide](docs/RFC_LOG_INTERPRETATION.md)**

The guide explains:
- How to verify RFC is working correctly
- What different log messages mean
- Common scenarios and troubleshooting steps
- Example log analysis

## Building

```bash
./configure
make
make install
```

## Testing

```bash
./run_ut.sh    # Run unit tests
./run_l2.sh    # Run L2 tests
```

## Configuration

RFC behavior is configured via:
- `/etc/rfc.properties` - Default properties
- `/opt/rfc.properties` - Override properties (non-production builds)

Key properties:
- `RFC_PATH` - Directory where RFC files are stored (default: `/opt/secure/RFC/`)
- `RFC_WRITE_LOCK` - Lock file to prevent concurrent writes

## File Locations

- **Configuration files**: `/opt/secure/RFC/.RFC_<FEATURE>.ini`
- **All variables**: `/opt/secure/RFC/rfcVariable.ini`
- **Logs**: `/opt/logs/rfcscript.log`

## Documentation

- [RFC Log Interpretation Guide](docs/RFC_LOG_INTERPRETATION.md) - Understand what RFC logs mean and troubleshoot issues
- [CHANGELOG](CHANGELOG.md) - Version history and release notes
- [CONTRIBUTING](CONTRIBUTING.md) - How to contribute to the project

## License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

## Support

For issues and questions:
- Create an issue on GitHub: https://github.com/rdkcentral/rfc/issues
- Check existing documentation in the `docs/` directory
- Review the log interpretation guide for common scenarios
