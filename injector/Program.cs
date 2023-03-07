/*
 * Copyright (c) 2017-2023 NeKz
 *
 * SPDX-License-Identifier: MIT
 */

using System;
using System.Diagnostics;
using System.IO;
using System.Text;

var defaultProcs = new string[] { "GridGame" };
var defaultLib = "tem.dll";

static string Inject(Process proc, string dllPath)
{
    try
    {
        if (!File.Exists(dllPath))
            return $"Cannot find dll named {dllPath}!";

        var kernel = Kernel32.LoadLibraryA("kernel32.dll");
        var load = Kernel32.GetProcAddress(kernel, "LoadLibraryA");
        Kernel32.FreeLibrary(kernel);
        if (load == UIntPtr.Zero)
            return "Cannot get LoadLibraryA!";

        var handle = Kernel32.OpenProcess(ProcessAccess.AllAccess, false, proc.Id);
        if (handle == IntPtr.Zero)
            return "Cannot open process!";

        var bytes = Encoding.ASCII.GetBytes(dllPath);
        var parameter = Kernel32.VirtualAllocEx(handle, IntPtr.Zero, (uint)bytes.Length, (uint)VirtualAllocExTypes.MEM_COMMIT_OR_RESERVE, (uint)PageProtection.PAGE_READWRITE);
        if (parameter == IntPtr.Zero)
            return "Cannot allocate memory!";

        if (!Kernel32.WriteProcessMemory(handle, parameter, bytes, (uint)bytes.Length, out _))
            return "Cannot write memory to process!";

        if (Kernel32.CreateRemoteThread(handle, IntPtr.Zero, 0u, load, parameter, 0u, out _) == IntPtr.Zero)
            return "Cannot open remote thread! GetLastError: " + Kernel32.GetLastError();
    }
    catch (Exception ex)
    {
        return ex.ToString();
    }
    return null;
}

var dll = Path.Combine(Path.GetDirectoryName(typeof(Program).Assembly.Location), defaultLib);

// Parse optional arguments
var argProc = string.Empty;
if (args.Length == 2)
{
    if (!string.IsNullOrEmpty(args[0]))
        argProc = args[0];
    if (args[1].EndsWith(".dll"))
        dll = Path.Combine(Path.GetDirectoryName(typeof(Program).Assembly.Location), args[1]);
}
else if (args.Length == 1)
{
    if (args[0].EndsWith(".dll"))
        dll = Path.Combine(Path.GetDirectoryName(typeof(Program).Assembly.Location), args[0]);
    else if (!string.IsNullOrEmpty(args[0]))
        argProc = args[0];
}

// Find process
var proc = default(Process);
if (!string.IsNullOrEmpty(argProc))
{
    proc = Array.Find(Process.GetProcesses(), p => p.ProcessName == argProc);
}
else
{
    foreach (var supportedProc in defaultProcs)
    {
        proc = Array.Find(Process.GetProcesses(), p => p.ProcessName == supportedProc);
        if (proc == null)
            continue;
        break;
    }
}

if (proc != null)
{
    var result = Inject(proc, dll);
    if (!string.IsNullOrEmpty(result))
    {
        Console.WriteLine($"Error: {result}");
        Console.ReadLine();
        return 1;
    }
}
else
{
    Console.WriteLine("Could not find supported process!");
    Console.ReadLine();
    return 1;
}


Console.WriteLine($"Successfully injected {dll} into process {proc.ProcessName}!");
return 0;
