# xqm.py - Cliente XQMdata: API de qmclient.py sobre xqmclilib.dll
#
# Uso:
#   import sys
#   sys.path.insert(0, r"C:\X_DESARROLLO\OPENCODE\XQM\XQMdata\win")
#   import xqm as qm
#   qm.Connect("localhost", 4243, "julio", "password", "WGES")
#   data, err = qm.Execute("COUNT VOC")
#
# Funciona reutilizando el qmclient.py oficial (firma de funciones) pero
# cargando nuestra xqmclilib.dll en lugar de la qmclilib.dll de OpenQM.

import os
import sys
import ctypes as ct

_HERE = os.path.dirname(os.path.abspath(__file__))

# qmclient.py (API) - del kit QMClient de referencia
_QMCLIENT_DIR = r"C:\X_DESARROLLO\OPENCODE\XQM\QM_WINDOWS\QM_CLIENT_4.0.8"
if _QMCLIENT_DIR not in sys.path:
    sys.path.insert(0, _QMCLIENT_DIR)

import qmclient as _qm

# Nuestra DLL (xqmclilib.dll) en lugar de la qmclilib.dll oficial
_DLL = os.path.join(_HERE, "obj", "xqmclilib.dll")
_qm.__qm_lib = ct.cdll.LoadLibrary(_DLL)


def __getattr__(name):
    """Re-exporta cualquier funcion de qmclient (Connect, Execute, Call, ...)."""
    return getattr(_qm, name)
