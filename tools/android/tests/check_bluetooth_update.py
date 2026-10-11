#!/usr/bin/env python3
"""Exercise production Bluetooth permission and shutdown boundaries with service stubs."""
from pathlib import Path
import os, subprocess, tempfile

repo = Path(__file__).resolve().parents[3]
java = Path(os.environ.get('JAVA_HOME', str(repo.parent/'toolchain/host/usr/lib/jvm/java-17-openjdk-amd64')))
ble = (repo/'android/app/src/main/java/org/libsdl/app/HIDDeviceBLESteamController.java').read_text()
manager = (repo/'android/app/src/main/java/org/libsdl/app/HIDDeviceManager.java').read_text()

def block(text, signature):
    a = text.index(signature); i = text.index('{', a)+1; depth = 1
    while depth:
        if text[i] == '{': depth += 1
        elif text[i] == '}': depth -= 1
        i += 1
    return text[a:i]

source = r'''import java.util.*;
public class BluetoothTest {
 static int checks;
 static void ok(boolean b,String message){if(!b)throw new AssertionError(message);checks++;}
 static class AppLog{static void w(String t,String m,Throwable e){}static void e(String t,String m){}}
 static class Build{static class VERSION{static int SDK_INT=35;}}
 static class Context{}
 static class HIDDeviceManager{Context getContext(){return new Context();}}
 static class Handler{boolean removed;void removeCallbacksAndMessages(Object ignored){removed=true;}}
 static class BluetoothGattDescriptor{
  static final byte[] ENABLE_NOTIFICATION_VALUE={1},ENABLE_INDICATION_VALUE={2};
  void setValue(byte[] value){}
 }
 static class BluetoothGattCharacteristic{
  static final int PROPERTY_NOTIFY=16,PROPERTY_INDICATE=32;
  BluetoothGattDescriptor getDescriptor(UUID uuid){return new BluetoothGattDescriptor();}
  int getProperties(){return PROPERTY_NOTIFY;}void setValue(byte[] value){}
 }
 static class BluetoothGattService{
  BluetoothGattCharacteristic chr=new BluetoothGattCharacteristic();
  BluetoothGattCharacteristic getCharacteristic(UUID uuid){return chr;}
 }
 static class BluetoothGatt{
  String denied="";int disconnects,closes;BluetoothGattService service=new BluetoothGattService();
  void permission(String call){if(denied.equals(call))throw new SecurityException(call);}
  BluetoothGattService getService(UUID uuid){return service;}
  boolean readCharacteristic(BluetoothGattCharacteristic c){permission("read");return true;}
  boolean writeCharacteristic(BluetoothGattCharacteristic c){permission("write");return true;}
  boolean setCharacteristicNotification(BluetoothGattCharacteristic c,boolean on){permission("notify");return true;}
  boolean writeDescriptor(BluetoothGattDescriptor d){permission("descriptor");return true;}
  void disconnect(){disconnects++;permission("disconnect");}
  void close(){closes++;permission("close");}
 }
 static class BluetoothDevice{
  static final int DEVICE_TYPE_LE=2;String name="SteamController";boolean denied;int connections;
  String getName(){if(denied)throw new SecurityException("name");return name;}
  int getType(){if(denied)throw new SecurityException("type");return DEVICE_TYPE_LE;}
  BluetoothGatt connectGatt(Context c,boolean managed,Object callback,int transport){connections++;if(denied)throw new SecurityException("connect");return new BluetoothGatt();}
  BluetoothGatt connectGatt(Context c,boolean managed,Object callback){connections++;if(denied)throw new SecurityException("connect");return new BluetoothGatt();}
 }
 static final String TAG="hidapi";
 static final UUID steamControllerService=UUID.randomUUID();
 static final int TRANSPORT_LE=2,D0G_BLE2_PID=0x1106,TRITON_BLE_PID=0x1303;
 volatile HIDDeviceManager mManager=new HIDDeviceManager();
 BluetoothDevice mDevice=new BluetoothDevice();volatile BluetoothGatt mGatt=new BluetoothGatt();
 Handler mHandler=new Handler();boolean mIsRegistered=true,mIsConnected=true;int mProductId=-1;
 LinkedList<GattOperation> mOperations=new LinkedList<>();GattOperation mCurrentOperation;
 public void close(){}
 GATT_OPERATION
 CONNECT_GATT
 PRODUCT_ID
 SHUTDOWN
 IS_STEAM
 public static void main(String[] args){
  BluetoothGatt g=new BluetoothGatt();UUID uuid=UUID.randomUUID();
  for(String denied:new String[]{"read","write","notify","descriptor"}){
   g.denied=denied;
   GattOperation op=denied.equals("read")?GattOperation.readCharacteristic(g,uuid):denied.equals("write")?GattOperation.writeCharacteristic(g,uuid,new byte[]{1}):GattOperation.enableNotification(g,uuid);
   op.run();ok(!op.finish(),"permission rejection returns failed operation: "+denied);
  }
  g.denied="";GattOperation op=GattOperation.readCharacteristic(g,uuid);op.run();ok(op.finish(),"permitted read still succeeds");
  g.service.chr=null;op=GattOperation.writeCharacteristic(g,uuid,new byte[]{1});op.run();ok(!op.finish(),"missing characteristic fails without null dereference");
  BluetoothTest t=new BluetoothTest();t.mDevice.denied=true;ok(t.connectGatt(false)==null && t.mDevice.connections==1,"denied connect is not retried using transport fallback");
  ok(!t.isSteamController(t.mDevice),"device detection handles denied name access");
  t.mDevice.denied=false;t.mDevice.name=null;ok(!t.isSteamController(t.mDevice) && t.getProductId()==D0G_BLE2_PID,"unnamed controller does not crash");
  t.mDevice.name="Steam Ctrl 2";t.mProductId=-1;ok(t.isSteamController(t.mDevice) && t.getProductId()==TRITON_BLE_PID,"permitted controller retains product identification");
  g=new BluetoothGatt();g.denied="disconnect";t.mGatt=g;t.mCurrentOperation=GattOperation.readCharacteristic(g,uuid);t.mOperations.add(t.mCurrentOperation);t.shutdown();
  ok(g.disconnects==1 && g.closes==1,"shutdown attempts close even if permission was revoked before disconnect");
  ok(t.mGatt==null && t.mManager==null && t.mCurrentOperation==null && t.mOperations.isEmpty() && t.mHandler.removed && !t.mIsRegistered && !t.mIsConnected,"shutdown cancels pending work and releases state");
  ok(t.connectGatt(false)==null,"late reconnect after shutdown is ignored");
  System.out.println("Bluetooth permission/shutdown checks passed: "+checks);
 }
}'''
for token, text in {
    'GATT_OPERATION': block(ble, 'static class GattOperation'),
    'CONNECT_GATT': block(ble, 'private BluetoothGatt connectGatt(boolean managed)'),
    'PRODUCT_ID': block(ble, 'public int getProductId()'),
    'SHUTDOWN': block(ble, 'public void shutdown()'),
    'IS_STEAM': block(manager, 'boolean isSteamController(BluetoothDevice bluetoothDevice)'),
}.items(): source = source.replace(token, text)
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory); path = root/'BluetoothTest.java'; path.write_text(source)
    subprocess.run([str(java/'bin/javac'), '-d', str(root), str(path)], check=True, timeout=30)
    subprocess.run([str(java/'bin/java'), '-cp', str(root), 'BluetoothTest'], check=True, timeout=15)
