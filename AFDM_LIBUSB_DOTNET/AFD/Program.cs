using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using LibUsbDotNet;
using LibUsbDotNet.Main;

namespace LibUSBDemo
{
    class LibUSB
    {
        private UsbDevice usbDevice;
        private UsbEndpointReader epReader;
        private UsbEndpointWriter epWriter;

         public void Open(int vid, int pid)
        {
            UsbDeviceFinder usbFinder = new UsbDeviceFinder(vid, pid);
            usbDevice = UsbDevice.OpenUsbDevice(usbFinder);


            if (usbDevice == null)
            {
                // 多次尝试连接USB设备
                int count = 0;
                while (count < 10 && usbDevice == null)
                {
                    Console.WriteLine(count);
                    Thread.Sleep(100);
                    usbDevice = UsbDevice.OpenUsbDevice(usbFinder);
                    count++;
                }
            }

            if (usbDevice == null)
            {
                Console.WriteLine("打开USB设备失败");
            }
            else
            {
                // If this is a "whole" usb device (libusb-win32, linux libusb)
                // it will have an IUsbDevice interface. If not (WinUSB) the 
                // variable will be null indicating this is an interface of a 
                // device.
                IUsbDevice wholeUSBDevice = usbDevice as IUsbDevice;
                if (wholeUSBDevice != null)
                {
                    // This is a "whole" USB device. Before it can be used, 
                    // the desired configuration and interface must be selected.

                    // Select config #1
                    wholeUSBDevice.SetConfiguration(1);

                    // Claim interface #0.
                    wholeUSBDevice.ClaimInterface(0);
                }

                // open read endpoint 1.
                epReader = usbDevice.OpenEndpointReader(ReadEndpointID.Ep01);

                // open write endpoint 1.
                epWriter = usbDevice.OpenEndpointWriter(WriteEndpointID.Ep01);
            }
        }


        public void Close()
        {
            if (IsOpen())
            {
                // If this is a "whole" usb device(libusb-win32, linux libusb-1.0)
                // it exposes an IUsbDevice interface. If not (WinUSB) the 
                // 'wholeUsbDevice' variable will be null indicating this is 
                // an interface of a device; it does not require or support 
                // configuration and interface selection.
                IUsbDevice wholeUsbDevice = usbDevice as IUsbDevice;
                if (!ReferenceEquals(wholeUsbDevice, null))
                {
                    // Release interface #0.
                    wholeUsbDevice.ReleaseInterface(0);
                }

                usbDevice.Close();
                usbDevice = null;
            }

            // Free usb resource
            UsbDevice.Exit();
        }

        public bool IsOpen()
        {
            if (usbDevice == null)
            {
                return false;
            }

            return usbDevice.IsOpen;
        }

        public List<byte> SendCommand(string cmd)
        {
            List<byte> result = new List<byte>();
            int byteWrite;
            ErrorCode ec = epWriter.Write(Encoding.Default.GetBytes(cmd), 3000,out byteWrite);
            if(ec != ErrorCode.None)
            {
                Console.WriteLine($"Write Error, {UsbDevice.LastErrorString}");
                return null;
            }

            byte[] readBuffer = new byte[1024];
            int byteRead;

            while (ec == ErrorCode.None)
            {
                // If the device hasn't sent data in the last 1000 milliseconds,
                // a timeout error (ec = IoTimedOut) will occur. 
                ec = epReader.Read(readBuffer, 1000, out byteRead);
                // 只有当超时的时候才会有byteRead为0，也就是结束
                if(byteRead != 0)
                {
                    byte[] buffer = new byte[byteRead];
                    Array.Copy(readBuffer, buffer, byteRead);
                    result.AddRange(buffer);
                }
                else
                {
                    Console.WriteLine("读取数据结束");
                    return result;
                }
            }
            return null;
        }

        static void Main()
        {
LibUSB usb = new();
usb.Open(0x0483,0x5740);
Console.WriteLine("Success");
if(usb.epWriter == null)
Console.WriteLine("epo1 out");
var k = usb.epWriter.Write(Encoding.Default.GetBytes("hello"), 100,out var byteWrite);
Console.WriteLine(k);
        }
    }
}

