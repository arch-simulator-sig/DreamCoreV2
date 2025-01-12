﻿using DreamCoreV2_model_controller.DataSource;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;

namespace DreamCoreV2_model_controller
{
    public static class Global
    {
        public static ValueDataSource<bool> connected = false;
        public static TcpClient? conn = null;
        public static Thread? recvThread = null;
        public static Thread? recvHandleThread = null;
        public static Thread? heartThread = null;
        //public static BlockingCollection<string> recvQueue = new BlockingCollection<string>();
        public static ConcurrentQueue<string> recvQueue = new ConcurrentQueue<string>();
        private static bool recvThreadClose = false;
        private static bool recvHandleThreadClose = false;
        private static bool heartThreadClose = false;
        private static long heartLastRecvTime = 0;

        public static ValueDataSource<bool> running = false;
        public static ValueDataSource<int> cpuCycle = 0;
        public static ValueDataSource<uint> pc = 0;

        public static ObjectDataSource<string> model_mode = "unknown";

        public delegate void CommandReceivedHandler(string prefix, string cmd, string result);
        public static event CommandReceivedHandler? CommandReceivedEvent;

        public static void Error(string message)
        {
            MessageBox.Show(message, "Error", MessageBoxButton.OK, MessageBoxImage.Error);
        }

        public static uint Onehot2Binary(uint onehot)
        {
            for(var i = 0;i < 32;i++)
            {
                if((onehot & (1 << i)) != 0)
                {
                    return (uint)(i + 1);
                }
            }
            
            return 0U;
        }

        private static object conn_stream_lock = new object();

        public static bool SendCommand(string prefix, string cmd, bool ignoreRunning = false)
        {
            if(!connected)
            {
                return false;
            }

            if(conn == null)
            {
                return false;
            }

            if(running && (cmd != "pause") && (cmd != "p") && (!ignoreRunning))
            {
                return false;
            }

            var str = prefix + " " + cmd;
            var bytes = Encoding.UTF8.GetBytes(str);
            var send_bytes = Encoding.Convert(Encoding.UTF8, Encoding.GetEncoding("GB2312"), bytes);
            
            lock(conn_stream_lock)
            {
                var stream = conn.GetStream();
                stream.Write(BitConverter.GetBytes((uint)send_bytes.Length), 0, 4);
                stream.Write(send_bytes, 0, send_bytes.Length);
            }
            
            return true;
        }

        public static void StartRecv()
        {
            recvThreadClose = false;
            recvThread = new Thread(RecvThreadEntry);
            recvThread.IsBackground = true;
            recvThread.Start();
            recvHandleThreadClose = false;
            recvHandleThread = new Thread(RecvHandleThreadEntry);
            recvHandleThread.IsBackground = true;
            recvHandleThread.Start();
            heartThreadClose = false;
            heartThread = new Thread(HeartThreadEntry);
            heartThread.IsBackground = true;
            heartThread.Start();
        }

        public static void StopRecv()
        {
            if(recvHandleThread != null)
            {
                recvHandleThreadClose = true;
                //recvQueue.Add("protocol none");
                recvHandleThread.Join();
                recvHandleThread = null;
            }

            if(recvThread != null)
            {
                recvThreadClose = true;
                recvThread.Join();
                recvThread = null;
            }
        }

        private static void RecvHandleThreadEntry()
        {
            while(!recvHandleThreadClose)
            {
                try
                {
                    /*var packet = recvQueue.Take();
                    var cmdArgList = packet.Split(' ');
                    var prefix = cmdArgList[0];
                    var cmd = cmdArgList[1];
                    var result = packet.Substring(prefix.Length + cmd.Length + 2);

                    if(prefix == "protocol")
                    {
                        if(cmd == "heart")
                        {
                            heartLastRecvTime = Environment.TickCount64;
                        }
                    }
                    else
                    {
                        CommandReceivedEvent?.Invoke(prefix, cmd, result);
                    }*/
                    if(!recvQueue.IsEmpty)
                    {
                        var packet = "";

                        if(recvQueue.TryDequeue(out packet))
                        {
                            var cmdArgList = packet.Split(' ');
                            var prefix = cmdArgList[0];
                            var cmd = cmdArgList[1];
                            var result = packet.Substring(prefix.Length + cmd.Length + 2);
                            
                            if(prefix == "protocol")
                            {
                                if(cmd == "heart")
                                {
                                    heartLastRecvTime = Environment.TickCount64;
                                }
                            }
                            else
                            {
                                CommandReceivedEvent?.Invoke(prefix, cmd, result);
                            }
                        }
                    }
                    else
                    {
                        Thread.Sleep(10);
                    }
                }
                catch
                {
                }
            }
        }

        private static void RecvThreadEntry()
        {
            var revLength = 0;
            byte[]? packetPayload = null;
            byte[] packetLength = new byte[4];

            while(!recvThreadClose)
            {
                try
                {
                    if((conn != null) && (connected))
                    {
                        var stream = conn.GetStream();
                        stream.ReadTimeout = 1000;
                        
                        if(packetPayload == null)
                        {
                            revLength += stream.Read(packetLength, revLength, packetLength.Length - revLength);
                            var finish = revLength == packetLength.Length;

                            if(finish)
                            {
                                var length = BitConverter.ToInt32(packetLength);

                                if(length > 0)
                                {
                                    packetPayload = new byte[length];
                                    revLength = 0;
                                }
                            }
                        }
                        else
                        {
                            revLength += stream.Read(packetPayload, revLength, packetPayload.Length - revLength);
                            var finish = revLength == packetPayload.Length;

                            if(finish)
                            {
                                var str = Encoding.UTF8.GetString(Encoding.Convert(Encoding.GetEncoding("GB2312"), Encoding.UTF8, packetPayload));
                                //recvQueue.Add(str);
                                recvQueue.Enqueue(str);
                                packetPayload = null;
                                revLength = 0;
                            }
                        }
                    }
                }
                catch(IOException)
                {
                }
                catch(Exception e)
                {
                    connected.Value = false;
                }
            }
        }

        private static void HeartThreadEntry()
        {
            heartLastRecvTime = Environment.TickCount64;

            while(!heartThreadClose)
            {
                try
                {
                    if((conn != null) && (connected))
                    {
                        SendCommand("protocol", "heart", true);
                        
                        /*if(Environment.TickCount64 - heartLastRecvTime > 2000)
                        {
                            connected.Value = false;
                        }*/
                    }

                    Thread.Sleep(1000);
                }
                catch(Exception e)
                {
                    connected.Value = false;
                }
            }
        }
    }
}
