# Test QMClient contra XQMdata (qmteld puerto 4243)
# Uso: python test_qmclient.py [usuario] [password] [cuenta]
#   Por defecto: usuario=julio, cuenta=WGES
import sys
sys.path.insert(0, r"C:\X_DESARROLLO\OPENCODE\XQM\QM_WINDOWS\QM_CLIENT_4.0.8")
import qmclient as qm

user = sys.argv[1] if len(sys.argv) > 1 else "julio"
pwd = sys.argv[2] if len(sys.argv) > 2 else ""
account = sys.argv[3] if len(sys.argv) > 3 else "WGES"

print(f"Conectando a localhost:4243 (usuario={user}, cuenta={account})...")
if not qm.Connect("localhost", 4243, user, pwd, account):
    print("FALLO Connect:", qm.Error())
    sys.exit(1)
print("Connect OK. Connected() =", qm.Connected())

print("Execute('W.CLIENTES')...")
data, err = qm.Execute("W.CLIENTES")
print("  respuesta:", repr(data), "err:", err)

qm.Disconnect()
print("Desconectado. TEST QMCLIENT OK")
