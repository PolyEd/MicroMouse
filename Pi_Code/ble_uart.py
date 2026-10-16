"""BLE GATT "UART" telemetry link using bluezero (BlueZ D-Bus).

Advertises the Nordic UART Service — the same service the Bluefruit LE
board exposed — and notifies the latest comma-separated telemetry line to
any connected central. The GATT server runs on a background thread; the
control loop only ever calls send_line(), which never blocks.
"""
import logging
import threading

import config

# Nordic UART Service UUIDs
UART_SERVICE = '6e400001-b5a3-f393-e0a9-e50e24dcca9e'
TX_CHARACTERISTIC = '6e400003-b5a3-f393-e0a9-e50e24dcca9e'


class BleUart:
    def __init__(self, local_name, tx_interval):
        self._local_name = local_name
        self._tx_interval = tx_interval
        self._latest = b''
        self._lock = threading.Lock()
        self._thread = None

    def send_line(self, line):
        """Publish the latest telemetry line. Non-blocking."""
        with self._lock:
            self._latest = line.encode('utf-8')

    def _get_latest(self):
        with self._lock:
            return list(self._latest)

    def _update(self, characteristic):
        """Timer callback: push the latest line out as a notification.
        Returning False (when the client unsubscribed) stops the timer."""
        value = self._get_latest()
        if value:
            characteristic.set_value(value)
        return characteristic.is_notifying

    def _notify_callback(self, notifying, characteristic):
        # Called when the client subscribes/unsubscribes.
        if notifying:
            from bluezero import async_tools
            async_tools.add_timer_seconds(self._tx_interval,
                                          self._update, characteristic)

    def start(self):
        if self._thread is not None:
            return
        self._thread = threading.Thread(target=self._run,
                                       name='ble-uart', daemon=True)
        self._thread.start()

    def _run(self):
        # Import here so the rest of the program still runs (with a
        # warning) if BlueZ or D-Bus is unavailable.
        try:
            from bluezero import adapter, peripheral
        except ImportError as exc:
            logging.warning('BLE telemetry unavailable (bluezero not '
                            'installed): %s', exc)
            return

        try:
            adapters = list(adapter.Adapter.available())
            if not adapters:
                logging.warning('BLE telemetry unavailable: no Bluetooth '
                                'adapter found')
                return
            adapter_address = adapters[0].address

            uart = peripheral.Peripheral(adapter_address,
                                         local_name=self._local_name)
            uart.add_service(srv_id=1, uuid=UART_SERVICE, primary=True)
            uart.add_characteristic(srv_id=1, chr_id=1,
                                    uuid=TX_CHARACTERISTIC,
                                    value=[], notifying=False,
                                    flags=['read', 'notify'],
                                    read_callback=self._get_latest,
                                    write_callback=None,
                                    notify_callback=self._notify_callback)
            logging.info('BLE telemetry advertising as "%s"',
                         self._local_name)
            uart.publish()  # runs the D-Bus main loop until exit
        except Exception as exc:
            logging.warning('BLE telemetry unavailable: %s', exc)
