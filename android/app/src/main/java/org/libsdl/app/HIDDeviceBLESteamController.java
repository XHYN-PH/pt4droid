package org.libsdl.app;

import android.content.Context;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothGattService;
import android.hardware.usb.UsbDevice;
import android.os.Handler;
import android.os.Looper;
import com.ptport.app.AppLog;
import android.os.*;

//import com.android.internal.util.HexDump;

import java.lang.Runnable;
import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.UUID;

import java.util.regex.Pattern;
import java.util.regex.Matcher;

class HIDDeviceBLESteamController extends BluetoothGattCallback implements HIDDevice {

    private static final String TAG = "hidapi";
    private volatile HIDDeviceManager mManager;
    private BluetoothDevice mDevice;
    private int mDeviceId;
    private volatile BluetoothGatt mGatt;
    private boolean mIsRegistered = false;
    private boolean mIsConnected = false;
    private boolean mIsChromebook = false;
    private boolean mIsReconnecting = false;
    private boolean mHasEnabledNotifications = false;
    private boolean mHasSeenInputUpdate = false;
    private boolean mFrozen = false;
    private LinkedList<GattOperation> mOperations;
    GattOperation mCurrentOperation = null;
    private Handler mHandler;
    private int mProductId = -1;
    private int mReportId = 0;
    private UUID mInputCharacteristic;

    private static final int D0G_BLE2_PID = 0x1106;
    private static final int TRITON_BLE_PID = 0x1303;


    private static final int TRANSPORT_AUTO = 0;
    private static final int TRANSPORT_BREDR = 1;
    private static final int TRANSPORT_LE = 2;

    private static final int CHROMEBOOK_CONNECTION_CHECK_INTERVAL = 10000;

    static final UUID steamControllerService = UUID.fromString("100F6C32-1735-4313-B402-38567131E5F3");
    static final UUID inputCharacteristicD0G = UUID.fromString("100F6C33-1735-4313-B402-38567131E5F3");
    static final UUID inputCharacteristicTriton_0x45 = UUID.fromString("100F6C7A-1735-4313-B402-38567131E5F3");
    static final UUID inputCharacteristicTriton_0x47 = UUID.fromString("100F6C7C-1735-4313-B402-38567131E5F3");
    static final UUID reportCharacteristic = UUID.fromString("100F6C34-1735-4313-B402-38567131E5F3");
    static private final byte[] enterValveMode = new byte[] { (byte)0xC0, (byte)0x87, 0x03, 0x08, 0x07, 0x00 };

    private HashMap<Integer, BluetoothGattCharacteristic> mOutputReportChars = new HashMap<Integer, BluetoothGattCharacteristic>();

    static class GattOperation {
        private enum Operation {
            CHR_READ,
            CHR_WRITE,
            ENABLE_NOTIFICATION
        }

        Operation mOp;
        UUID mUuid;
        byte[] mValue;
        BluetoothGatt mGatt;
        boolean mResult = true;
        int mDelayMs = 0;

        private GattOperation(BluetoothGatt gatt, GattOperation.Operation operation, UUID uuid) {
            mGatt = gatt;
            mOp = operation;
            mUuid = uuid;
        }

        private GattOperation(BluetoothGatt gatt, GattOperation.Operation operation, UUID uuid, int delayMs) {
            mGatt = gatt;
            mOp = operation;
            mUuid = uuid;
            mDelayMs = delayMs;
        }

        private GattOperation(BluetoothGatt gatt, GattOperation.Operation operation, UUID uuid, byte[] value) {
            mGatt = gatt;
            mOp = operation;
            mUuid = uuid;
            mValue = value;
        }

        private GattOperation(BluetoothGatt gatt, GattOperation.Operation operation, UUID uuid, byte[] value, int delayMs) {
            mGatt = gatt;
            mOp = operation;
            mUuid = uuid;
            mValue = value;
            mDelayMs = delayMs;
        }

        public void run() {
            if (mGatt == null) { mResult = false; return; }
            try {
                // This is executed in main thread
                BluetoothGattCharacteristic chr;

                switch (mOp) {
                    case CHR_READ:
                        chr = getCharacteristic(mUuid);
                        if (chr == null) { mResult = false; return; }
                        //AppLog.v(TAG, "Reading characteristic " + chr.getUuid());
                        if (!mGatt.readCharacteristic(chr)) {
                            AppLog.e(TAG, "Unable to read characteristic " + mUuid.toString());
                            mResult = false;
                            break;
                        }
                        mResult = true;
                        break;
                    case CHR_WRITE:
                        chr = getCharacteristic(mUuid);
                        if (chr == null) { mResult = false; return; }
                        //AppLog.v(TAG, "Writing characteristic " + chr.getUuid() + " value=" + HexDump.toHexString(value));
                        chr.setValue(mValue);
                        if (!mGatt.writeCharacteristic(chr)) {
                            AppLog.e(TAG, "Unable to write characteristic " + mUuid.toString());
                            mResult = false;
                            break;
                        }
                        mResult = true;
                        break;
                    case ENABLE_NOTIFICATION:
                        chr = getCharacteristic(mUuid);
                        if (chr == null) { mResult = false; return; }
                        //AppLog.v(TAG, "Writing descriptor of " + chr.getUuid());
                        if (chr != null) {
                            BluetoothGattDescriptor cccd = chr.getDescriptor(UUID.fromString("00002902-0000-1000-8000-00805f9b34fb"));
                            if (cccd != null) {
                                int properties = chr.getProperties();
                                byte[] value;
                                if ((properties & BluetoothGattCharacteristic.PROPERTY_NOTIFY) == BluetoothGattCharacteristic.PROPERTY_NOTIFY) {
                                    value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE;
                                } else if ((properties & BluetoothGattCharacteristic.PROPERTY_INDICATE) == BluetoothGattCharacteristic.PROPERTY_INDICATE) {
                                    value = BluetoothGattDescriptor.ENABLE_INDICATION_VALUE;
                                } else {
                                    AppLog.e(TAG, "Unable to start notifications on input characteristic");
                                    mResult = false;
                                    return;
                                }

                                mGatt.setCharacteristicNotification(chr, true);
                                cccd.setValue(value);
                                if (!mGatt.writeDescriptor(cccd)) {
                                    AppLog.e(TAG, "Unable to write descriptor " + mUuid.toString());
                                    mResult = false;
                                    return;
                                }
                                mResult = true;
                            }
                        }
                }

            } catch (SecurityException e) {
                AppLog.w(TAG, "Bluetooth permission unavailable", e);
                mResult = false;
            }
        }

        public boolean finish() {
            return mResult;
        }

        public int getDelayMs() { return mDelayMs; }

        private BluetoothGattCharacteristic getCharacteristic(UUID uuid) {
            BluetoothGattService valveService = mGatt.getService(steamControllerService);
            if (valveService == null)
                return null;
            return valveService.getCharacteristic(uuid);
        }

        static public GattOperation readCharacteristic(BluetoothGatt gatt, UUID uuid) {
            return new GattOperation(gatt, Operation.CHR_READ, uuid);
        }

        static public GattOperation writeCharacteristic(BluetoothGatt gatt, UUID uuid, byte[] value) {
            return new GattOperation(gatt, Operation.CHR_WRITE, uuid, value);
        }

        static public GattOperation enableNotification(BluetoothGatt gatt, UUID uuid) {
            return new GattOperation(gatt, Operation.ENABLE_NOTIFICATION, uuid);
        }

        static public GattOperation enableNotification(BluetoothGatt gatt, UUID uuid, int delayMs) {
            return new GattOperation(gatt, Operation.ENABLE_NOTIFICATION, uuid, delayMs);
        }
    }

    HIDDeviceBLESteamController(HIDDeviceManager manager, BluetoothDevice device) {
        mManager = manager;
        mDevice = device;
        mDeviceId = mManager.getDeviceIDForIdentifier(getIdentifier());
        mIsRegistered = false;
        mIsChromebook = SDLActivity.isChromebook();
        mOperations = new LinkedList<GattOperation>();
        mHandler = new Handler(Looper.getMainLooper());

        mGatt = connectGatt();
        mHasEnabledNotifications = false;
        mHasSeenInputUpdate = false;
        // final HIDDeviceBLESteamController finalThis = this;
        // mHandler.postDelayed(new Runnable() {
        //     @Override
        //     void run() {
        //         finalThis.checkConnectionForChromebookIssue();
        //     }
        // }, CHROMEBOOK_CONNECTION_CHECK_INTERVAL);
    }

    String getIdentifier() {
        return String.format("SteamController.%s", mDevice.getAddress());
    }

    BluetoothGatt getGatt() {
        return mGatt;
    }

    // Because on Chromebooks we show up as a dual-mode device, it will attempt to connect TRANSPORT_AUTO, which will use TRANSPORT_BREDR instead
    // of TRANSPORT_LE.  Let's force ourselves to connect low energy.
    private BluetoothGatt connectGatt(boolean managed) {
        if (mManager == null) return null;
        try {
            if (Build.VERSION.SDK_INT >= 23 /* Android 6.0 (M) */) {
                try {
                    return mDevice.connectGatt(mManager.getContext(), managed, this, TRANSPORT_LE);
                } catch (SecurityException e) {
                    throw e;
                } catch (Exception e) {
                    return mDevice.connectGatt(mManager.getContext(), managed, this);
                }
            } else {
                return mDevice.connectGatt(mManager.getContext(), managed, this);
            }

        } catch (SecurityException e) {
            AppLog.w(TAG, "Bluetooth permission unavailable", e);
            return null;
        }
    }

    private BluetoothGatt connectGatt() {
        return connectGatt(false);
    }

    protected int getConnectionState() {
        if (mManager == null || mGatt == null) return BluetoothProfile.STATE_DISCONNECTED;
        try {
            Context context = mManager.getContext();
            if (context == null) {
                // We are lacking any context to get our Bluetooth information.  We'll just assume disconnected.
                return BluetoothProfile.STATE_DISCONNECTED;
            }

            BluetoothManager btManager = (BluetoothManager)context.getSystemService(Context.BLUETOOTH_SERVICE);
            if (btManager == null) {
                // This device doesn't support Bluetooth.  We should never be here, because how did
                // we instantiate a device to start with?
                return BluetoothProfile.STATE_DISCONNECTED;
            }

            return btManager.getConnectionState(mDevice, BluetoothProfile.GATT);

        } catch (SecurityException e) {
            AppLog.w(TAG, "Bluetooth permission unavailable", e);
            return BluetoothProfile.STATE_DISCONNECTED;
        }
    }

    void reconnect() {
        if (mManager == null) return;
        try {
            if (getConnectionState() != BluetoothProfile.STATE_CONNECTED) {
                if (mGatt != null) mGatt.disconnect();
                mGatt = connectGatt();
            }


        } catch (SecurityException e) {
            AppLog.w(TAG, "Bluetooth permission unavailable", e);
        }
    }

    protected void checkConnectionForChromebookIssue() {
        if (mGatt == null || mManager == null) return;
        try {
            if (!mIsChromebook) {
                // We only do this on Chromebooks, because otherwise it's really annoying to just attempt
                // over and over.
                return;
            }

            int connectionState = getConnectionState();

            switch (connectionState) {
                case BluetoothProfile.STATE_CONNECTED:
                    if (!mIsConnected) {
                        // We are in the Bad Chromebook Place.  We can force a disconnect
                        // to try to recover.
                        AppLog.v(TAG, "Chromebook: We are in a very bad state; the controller shows as connected in the underlying Bluetooth layer, but we never received a callback.  Forcing a reconnect.");
                        mIsReconnecting = true;
                        mGatt.disconnect();
                        mGatt = connectGatt(false);
                        break;
                    }
                    else if (!isRegistered()) {
                        if (mGatt.getServices().size() > 0) {
                            AppLog.v(TAG, "Chromebook: We are connected to a controller, but never got our registration.  Trying to recover.");
                            probeService(this);
                        }
                        else {
                            AppLog.v(TAG, "Chromebook: We are connected to a controller, but never discovered services.  Trying to recover.");
                            mIsReconnecting = true;
                            mGatt.disconnect();
                            mGatt = connectGatt(false);
                            break;
                        }
                    }
                    else {
                        AppLog.v(TAG, "Chromebook: We are connected, and registered.  Everything's good!");
                        return;
                    }
                    break;

                case BluetoothProfile.STATE_DISCONNECTED:
                    AppLog.v(TAG, "Chromebook: We have either been disconnected, or the Chromebook BtGatt.ContextMap bug has bitten us.  Attempting a disconnect/reconnect, but we may not be able to recover.");

                    mIsReconnecting = true;
                    mGatt.disconnect();
                    mGatt = connectGatt(false);
                    break;

                case BluetoothProfile.STATE_CONNECTING:
                    AppLog.v(TAG, "Chromebook: We're still trying to connect.  Waiting a bit longer.");
                    break;
            }

            final HIDDeviceBLESteamController finalThis = this;
            mHandler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    finalThis.checkConnectionForChromebookIssue();
                }
            }, CHROMEBOOK_CONNECTION_CHECK_INTERVAL);

        } catch (SecurityException e) {
            AppLog.w(TAG, "Bluetooth permission unavailable", e);
        }
    }

    private boolean isRegistered() {
        return mIsRegistered;
    }

    private void setRegistered() {
        mIsRegistered = true;
    }

    private boolean probeService(HIDDeviceBLESteamController controller) {
        if (mGatt == null || mManager == null) return false;
        try {
            if (isRegistered()) {
                return true;
            }

            if (!mIsConnected) {
                return false;
            }

            AppLog.v(TAG, "probeService controller=" + controller);

            for (BluetoothGattService service : mGatt.getServices()) {
                if (service.getUuid().equals(steamControllerService)) {
                    AppLog.v(TAG, "Found Valve steam controller service " + service.getUuid());

                    for (BluetoothGattCharacteristic chr : service.getCharacteristics()) {
                        if (chr.getUuid().equals(inputCharacteristicTriton_0x45)) {
                            AppLog.v(TAG, "Found Triton input characteristic 0x45");
                            mProductId = TRITON_BLE_PID;
                            mReportId = 0x45;
                            mInputCharacteristic = chr.getUuid();
                        } else if (chr.getUuid().equals(inputCharacteristicTriton_0x47)) {
                            AppLog.v(TAG, "Found Triton input characteristic 0x47");
                            mProductId = TRITON_BLE_PID;
                            mReportId = 0x47;
                            mInputCharacteristic = chr.getUuid();
                        } else if (chr.getUuid().equals(inputCharacteristicD0G)) {
                            AppLog.v(TAG, "Found D0G input characteristic");
                            mProductId = D0G_BLE2_PID;
                            mReportId = 0x03;
                            mInputCharacteristic = chr.getUuid();
                        } else {
                            Pattern reportPattern = Pattern.compile("100F6C([0-9A-Z]{2})", Pattern.CASE_INSENSITIVE);
                            Matcher matcher = reportPattern.matcher(chr.getUuid().toString());

                            if (matcher.find()) {
                                try {
                                    int reportId = Integer.parseInt(matcher.group(1), 16);

                                    reportId -= 0x35;
                                    if (reportId >= 0x80) {
                                        // This is a Triton output report characteristic that we need to care about.
                                        AppLog.v(TAG, "Found Triton output report 0x" + Integer.toString(reportId, 16));
                                        mOutputReportChars.put(reportId, chr);
                                    }
                                }
                                catch (NumberFormatException nfe) {
                                    AppLog.w(TAG, "Could not parse report characteristic " + chr.getUuid().toString() + ": " + nfe.toString());
                                }
                            }
                        }
                    }

                    for (BluetoothGattCharacteristic chr : service.getCharacteristics()) {
                        if (chr.getUuid().equals(mInputCharacteristic)) {
                            // Start notifications
                            BluetoothGattDescriptor cccd = chr.getDescriptor(UUID.fromString("00002902-0000-1000-8000-00805f9b34fb"));
                            if (cccd != null) {
                                enableNotification(chr.getUuid());
                            }
                        }
                    }
                    return true;
                }
            }

            if ((mGatt.getServices().size() == 0) && mIsChromebook && !mIsReconnecting) {
                AppLog.e(TAG, "Chromebook: Discovered services were empty; this almost certainly means the BtGatt.ContextMap bug has bitten us.");
                mIsConnected = false;
                mIsReconnecting = true;
                mGatt.disconnect();
                mGatt = connectGatt(false);
            }

            return false;

        } catch (SecurityException e) {
            AppLog.w(TAG, "Bluetooth permission unavailable", e);
            return false;
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    private void finishCurrentGattOperation() {
        GattOperation op = null;
        synchronized (mOperations) {
            if (mCurrentOperation != null) {
                op = mCurrentOperation;
                mCurrentOperation = null;
            }
        }
        if (op != null) {
            boolean result = op.finish(); // TODO: Maybe in main thread as well?

            // Our operation failed, let's add it back to the beginning of our queue.
            if (!result) {
                mOperations.addFirst(op);
            }
        }
        executeNextGattOperation();
    }

    private void executeNextGattOperation() {
        synchronized (mOperations) {
            if (mCurrentOperation != null)
                return;

            if (mOperations.isEmpty())
                return;

            mCurrentOperation = mOperations.removeFirst();
        }

        Runnable gattOperationRunnable = new Runnable() {
                @Override
                public void run() {
                    synchronized (mOperations) {
                        if (mCurrentOperation == null) {
                            AppLog.e(TAG, "Current operation null in executor?");
                            return;
                        }

                        mCurrentOperation.run();
                        if (!mCurrentOperation.finish()) {
                            mCurrentOperation = null;
                            executeNextGattOperation();
                            return;
                        }
                        // now wait for the GATT callback and when it comes, finish this operation
                    }
                }
            };

        if (mCurrentOperation.getDelayMs() == 0) {
            // Run in main thread
            mHandler.post(gattOperationRunnable);
        }
        else {
            // If we have a delay on this operation, wait before we post it.
            mHandler.postDelayed(gattOperationRunnable, mCurrentOperation.getDelayMs());
        }

    }

    private void queueGattOperation(GattOperation op) {
        synchronized (mOperations) {
            mOperations.add(op);
        }
        executeNextGattOperation();
    }

    private void enableNotification(UUID chrUuid) {
        // Add a 500ms delay to notification write for Amazon Fire TV devices, as otherwise if we do this too quickly after connecting
        // it will return success and then silently drop the operation on the floor.
        GattOperation op = HIDDeviceBLESteamController.GattOperation.enableNotification(mGatt, chrUuid, 500);
        queueGattOperation(op);

        // Amazon Fire devices can also silently timeout on writeDescriptor, so
        // set up a little delayed check that will attempt to write a second time.
        //
        // While this only seems to be needed on Amazon Fire TV devices at present, it
        // doesn't hurt to have a retry on other devices as well.
        //
        final HIDDeviceBLESteamController finalThis = this;
        final UUID finalUuid = chrUuid;
        mHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                if (!finalThis.mHasEnabledNotifications) {

                    if (finalThis.mHasSeenInputUpdate) {
                        // Amazon Five devices may have enabled notifications on the input characteristic and not given us a callback. If we've seen
                        // input reports, though, somewhat by definition notifications are enabled.
                        AppLog.w(TAG, "WriteDescriptor has never returned, but we've seen input reports. Moving on with controller initialization.");
                        finalThis.mHasEnabledNotifications = true;
                        finalThis.enableValveMode();
                        return;
                    }

                    // Give one more try.
                    GattOperation retry = HIDDeviceBLESteamController.GattOperation.enableNotification(finalThis.mGatt, finalUuid, 500);
                    finalThis.queueGattOperation(retry);
                }
            }
        }, 1000);
    }

    void writeCharacteristic(UUID uuid, byte[] value) {
        GattOperation op = HIDDeviceBLESteamController.GattOperation.writeCharacteristic(mGatt, uuid, value);
        queueGattOperation(op);
    }

    void readCharacteristic(UUID uuid) {
        GattOperation op = HIDDeviceBLESteamController.GattOperation.readCharacteristic(mGatt, uuid);
        queueGattOperation(op);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////  BluetoothGattCallback overridden methods
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    @Override
    public void onConnectionStateChange(BluetoothGatt g, int status, int newState) {
        //AppLog.v(TAG, "onConnectionStateChange status=" + status + " newState=" + newState);
        if (mManager == null || g != mGatt) return;
        mIsReconnecting = false;
        if (newState == 2) {
            mIsConnected = true;
            // Run directly, without GattOperation
            if (!isRegistered()) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if (mManager == null || g != mGatt) return;
                        try {
                            g.discoverServices();
                        } catch (SecurityException e) {
                            AppLog.w(TAG, "Bluetooth permission unavailable", e);
                        }
                    }
                });
            }
        }
        else if (newState == 0) {
            mIsConnected = false;
        }

        // Disconnection is handled in SteamLink using the ACTION_ACL_DISCONNECTED Intent.
    }

    @Override
    public void onServicesDiscovered(BluetoothGatt gatt, int status) {
        if (mManager == null || gatt != mGatt) return;
        try {
            //AppLog.v(TAG, "onServicesDiscovered status=" + status);
            if (status == 0) {
                if (gatt.getServices().size() == 0) {
                    AppLog.v(TAG, "onServicesDiscovered returned zero services; something has gone horribly wrong down in Android's Bluetooth stack.");
                    mIsReconnecting = true;
                    mIsConnected = false;
                    gatt.disconnect();
                    mGatt = connectGatt(false);
                } else {
                    if (getProductId() == TRITON_BLE_PID) {
                        // Android will not properly play well with Data Length Extensions without manually requesting a large MTU,
                        // and Triton controllers require DLE support.
                        //
                        // 517 is basically a "magic number" as far as Android's bluetooth code is concerned, so do not change
                        // this value. It is functionally "please enable data length extensions" on some Android builds.
                        mGatt.requestMtu(517);
                    }

                    probeService(this);
                }
            }

        } catch (SecurityException e) {
            AppLog.w(TAG, "Bluetooth permission unavailable", e);
        }
    }

    @Override
    public void onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
        if (mManager == null || gatt != mGatt) return;
        //AppLog.v(TAG, "onCharacteristicRead status=" + status + " uuid=" + characteristic.getUuid());

        if (characteristic.getUuid().equals(reportCharacteristic) && !mFrozen) {
            mManager.HIDDeviceReportResponse(getId(), characteristic.getValue());
        }

        finishCurrentGattOperation();
    }

    @Override
    public void onCharacteristicWrite(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
        if (mManager == null || gatt != mGatt) return;
        //AppLog.v(TAG, "onCharacteristicWrite status=" + status + " uuid=" + characteristic.getUuid());

        if (characteristic.getUuid().equals(reportCharacteristic)) {
            // Only register controller with the native side once it has been fully configured
            if (!isRegistered()) {
                AppLog.v(TAG, "Registering Steam Controller with ID: " + getId());
                mManager.HIDDeviceConnected(getId(), getIdentifier(), getVendorId(), getProductId(), getSerialNumber(), getVersion(), getManufacturerName(), getProductName(), 0, 0, 0, 0, true, mReportId);
                setRegistered();
            }
        }

        finishCurrentGattOperation();
    }

    @Override
    public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
        if (mManager == null || gatt != mGatt) return;
    // Enable this for verbose logging of controller input reports
        //AppLog.v(TAG, "onCharacteristicChanged uuid=" + characteristic.getUuid() + " data=" + HexDump.dumpHexString(characteristic.getValue()));

        if (characteristic.getUuid().equals(mInputCharacteristic) && !mFrozen) {
            mHasSeenInputUpdate = true;
            mManager.HIDDeviceInputReport(getId(), characteristic.getValue());
        }
    }

    @Override
    public void onDescriptorRead(BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status) {
        //AppLog.v(TAG, "onDescriptorRead status=" + status);
    }

    private void enableValveMode()
    {
        if (mGatt == null || mManager == null) return;
        try {
            BluetoothGattService valveService = mGatt.getService(steamControllerService);
            if (valveService == null)
                return;

            BluetoothGattCharacteristic reportChr = valveService.getCharacteristic(reportCharacteristic);
            if (reportChr != null) {
                if (getProductId() == TRITON_BLE_PID) {
                    // For Triton we just mark things registered.
                    AppLog.v(TAG, "Registering Triton Steam Controller with ID: " + getId());
                    mManager.HIDDeviceConnected(getId(), getIdentifier(), getVendorId(), getProductId(), getSerialNumber(), getVersion(), getManufacturerName(), getProductName(), 0, 0, 0, 0, true, mReportId);
                    setRegistered();
                } else {
                    // For the original controller, we need to manually enter Valve mode.
                    AppLog.v(TAG, "Writing report characteristic to enter valve mode");
                    reportChr.setValue(enterValveMode);
                    mGatt.writeCharacteristic(reportChr);
                }
            }

        } catch (SecurityException e) {
            AppLog.w(TAG, "Bluetooth permission unavailable", e);
        }
    }

    @Override
    public void onDescriptorWrite(BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status) {
        if (mManager == null || gatt != mGatt) return;
        BluetoothGattCharacteristic chr = descriptor.getCharacteristic();
        //AppLog.v(TAG, "onDescriptorWrite status=" + status + " uuid=" + chr.getUuid() + " descriptor=" + descriptor.getUuid());

        if (chr.getUuid().equals(mInputCharacteristic)) {
            mHasEnabledNotifications = true;
            enableValveMode();
        }

        finishCurrentGattOperation();
    }

    @Override
    public void onReliableWriteCompleted(BluetoothGatt gatt, int status) {
        //AppLog.v(TAG, "onReliableWriteCompleted status=" + status);
    }

    @Override
    public void onReadRemoteRssi(BluetoothGatt gatt, int rssi, int status) {
        //AppLog.v(TAG, "onReadRemoteRssi status=" + status);
    }

    @Override
    public void onMtuChanged(BluetoothGatt gatt, int mtu, int status) {
        //AppLog.v(TAG, "onMtuChanged status=" + status);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////
    //////// Public API
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    @Override
    public int getId() {
        return mDeviceId;
    }

    @Override
    public int getVendorId() {
        // Valve Corporation
        final int VALVE_USB_VID = 0x28DE;
        return VALVE_USB_VID;
    }

    @Override
    public int getProductId() {
        try {
            if (mProductId > 0) {
                // We've already set a product ID.
                return mProductId;
            }

            String name = mDevice.getName();
        if (name != null && name.startsWith("Steam Ctrl")) {
                // We're a newer Triton device
                mProductId = TRITON_BLE_PID;
            } else {
                // We're an OG Steam Controller
                mProductId = D0G_BLE2_PID;
            }

            return mProductId;

        } catch (SecurityException e) {
            AppLog.w(TAG, "Bluetooth permission unavailable", e);
            return D0G_BLE2_PID;
        }
    }

    @Override
    public String getSerialNumber() {
        // This will be read later via feature report by Steam
        return "12345";
    }

    @Override
    public int getVersion() {
        return 0;
    }

    @Override
    public String getManufacturerName() {
        return "Valve Corporation";
    }

    @Override
    public String getProductName() {
        return "Steam Controller";
    }

    @Override
    public UsbDevice getDevice() {
        return null;
    }

    @Override
    public boolean open() {
        return true;
    }

    @Override
    public int writeReport(byte[] report, boolean feature) {
        if (!isRegistered()) {
            AppLog.e(TAG, "Attempted writeReport before Steam Controller is registered!");
            if (mIsConnected) {
                probeService(this);
            }
            return -1;
        }

        if (feature) {
            // We need to skip the first byte, as that doesn't go over the air
            byte[] actual_report = Arrays.copyOfRange(report, 1, report.length - 1);
            //AppLog.v(TAG, "writeFeatureReport " + HexDump.dumpHexString(actual_report));
            writeCharacteristic(reportCharacteristic, actual_report);
            return report.length;
        } else {
            // If we're an original-recipe Steam Controller we just write to the characteristic directly.
            if (getProductId() == D0G_BLE2_PID) {
                //AppLog.v(TAG, "writeOutputReport " + HexDump.dumpHexString(report));
                writeCharacteristic(reportCharacteristic, report);
                return report.length;
            }

            // If we're a Triton, we need to find the correct report characteristic.
            if (report.length > 0) {
                int reportId = report[0] & 0xFF;
                BluetoothGattCharacteristic targetedReportCharacteristic = mOutputReportChars.get(reportId);
                if (targetedReportCharacteristic != null) {
                    byte[] actual_report = Arrays.copyOfRange(report, 1, report.length - 1);
                    //AppLog.v(TAG, "writeOutputReport 0x" + Integer.toString(reportId, 16) + " " + HexDump.dumpHexString(report));
                    writeCharacteristic(targetedReportCharacteristic.getUuid(), actual_report);
                    return report.length;
                } else {
                    AppLog.w(TAG, "Got report write request for unknown report type 0x" + Integer.toString(reportId, 16));
                }
            }
        }

        return -1;
    }

    @Override
    public boolean readReport(byte[] report, boolean feature) {
        if (!isRegistered()) {
            AppLog.e(TAG, "Attempted readReport before Steam Controller is registered!");
            if (mIsConnected) {
                probeService(this);
            }
            return false;
        }

        if (feature) {
            readCharacteristic(reportCharacteristic);
            return true;
        } else {
            // Not implemented
            return false;
        }
    }

    @Override
    public void close() {
    }

    @Override
    public void setFrozen(boolean frozen) {
        mFrozen = frozen;
    }

    @Override
    public void shutdown() {
        mHandler.removeCallbacksAndMessages(null);
        close();
        BluetoothGatt g = mGatt;
        mGatt = null;
        mManager = null;
        mIsRegistered = false;
        mIsConnected = false;
        synchronized (mOperations) {
            mCurrentOperation = null;
            mOperations.clear();
        }
        if (g != null) {
            try { g.disconnect(); } catch (SecurityException e) {
                AppLog.w(TAG, "Bluetooth permission unavailable", e);
            }
            try { g.close(); } catch (SecurityException e) {
                AppLog.w(TAG, "Bluetooth permission unavailable", e);
            }
        }
    }

}
