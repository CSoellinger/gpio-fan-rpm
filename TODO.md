# Feature Ideas (by usefulness)

1. CSV Output Mode - Add --csv output for easy spreadsheet/data analysis integration.
2. Configurable Warmup Duration - Currently fixed at 1 second. Allow --warmup=N for fans with longer spin-up times.
3. Min/Max/Average Statistics in Watch Mode - Show running statistics: GPIO17: RPM: 5280 (min: 5100, max: 5400, avg: 5260)
4. Alarm/Threshold Mode - Alert if RPM drops below threshold: --min-rpm=1000 exits with error code if violated.
5. Systemd Integration - Add --daemon mode or systemd notify support for service integration.
6. Edge Type Selection - Currently uses GPIOD_LINE_EDGE_BOTH. Allow --edge=rising|falling|both for different tach signals.
7. Multiple Output Destinations - Support --output=/path/to/file to write to file while monitoring.
8. Prometheus/InfluxDB Output Format - Add --prometheus or --influx for metrics collectors.
9. Temperature Correlation - Accept temperature input to show RPM vs temperature graphs (piped from another sensor).
10. Config File Support - Read defaults from /etc/gpio-fan-rpm.conf or ~/.config/gpio-fan-rpm.conf
