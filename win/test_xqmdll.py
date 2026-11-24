# Test xqmclilib.dll (nuestra DLL) — carga y resolución de funciones
import sys, ctypes as ct

DLL = r"C:\X_DESARROLLO\OPENCODE\XQM\XQMdata\win\obj\xqmclilib.dll"
print("Cargando", DLL)
lib = ct.cdll.LoadLibrary(DLL)
print("  cargada OK")

# Resolución de símbolos clave
for name in ["QMConnectW", "QMCallxW", "QMExecuteW", "QMReadW",
             "QMConnectLocalW", "QMErrorW", "QMOpenW", "QMWriteW"]:
    f = getattr(lib, name)
    print("  ", name, "-> OK")

# Prueba de conexión real (QMConnectW)
host = "localhost"
user = sys.argv[1] if len(sys.argv) > 1 else "julio"
pwd = sys.argv[2] if len(sys.argv) > 2 else ""
acct = sys.argv[3] if len(sys.argv) > 3 else "WGES"

lib.QMConnectW.argtypes = [ct.c_wchar_p, ct.c_int, ct.c_wchar_p, ct.c_wchar_p, ct.c_wchar_p]
lib.QMErrorW.restype = ct.c_void_p

print(f"QMConnectW({host}:4243, {user}, cuenta={acct})...")
r = lib.QMConnectW(host, 4243, user, pwd, acct)
print("  resultado:", r)
if r:
    lib.QMExecuteW.restype = ct.c_void_p
    lib.QMExecuteW.argtypes = [ct.c_wchar_p, ct.POINTER(ct.c_int)]
    err = ct.c_int()
    s = lib.QMExecuteW("COUNT VOC", ct.byref(err))
    print("  COUNT VOC ->", ct.wstring_at(s), "err:", err.value)
    lib.QMFree(ct.c_void_p(s))
    lib.QMDisconnect()
    print("TEST DLL OK")
else:
    print("  error:", ct.wstring_at(lib.QMErrorW()))
    sys.exit(1)
