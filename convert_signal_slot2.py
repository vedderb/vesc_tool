#!/usr/bin/env python3
"""
Phase 2: Convert remaining multi-line SIGNAL/SLOT connections.
Character-based parser with comprehensive class mapping.
"""

import os

def guess_this_class(filepath, context=''):
    """Guess class for 'this' based on filepath and surrounding code context."""
    basename = os.path.basename(filepath).replace('.cpp', '')
    
    class_map = {
        'vescinterface': 'VescInterface', 'mainwindow': 'MainWindow',
        'boardsetupwindow': 'BoardSetupWindow', 'commands': 'Commands',
        'codeloader': 'CodeLoader', 'utility': 'Utility',
        'tcpserversimple': 'TcpServerSimple', 'udpserversimple': 'UdpServerSimple',
        'tcphub': 'TcpHub', 'packet': 'Packet', 'preferences': 'Preferences',
        'parametereditor': 'ParameterEditor', 'bleuart': 'BleUart',
        'mrichtextedit': 'MRichTextEdit', 'mapwidget': 'MapWidget',
        'osmclient': 'OsmClient', 'pagefirmware': 'PageFirmware',
        'pageconnection': 'PageConnection', 'pageswdprog': 'PageSwdProg',
        'pagesampleddata': 'PageSampledData', 'pagedisplaytool': 'PageDisplayTool',
        'pageterminal': 'PageTerminal', 'pageexperiments': 'PageExperiments',
        'pagewelcome': 'PageWelcome', 'pageimu': 'PageImu',
        'pageappimu': 'PageAppImu', 'pagertdata': 'PageRtData',
        'pagebms': 'PageBms', 'pageespprog': 'PageEspProg',
        'pagecananalyzer': 'PageCanAnalyzer', 'pagecustomconfig': 'PageCustomConfig',
        'pageappnunchuk': 'PageAppNunchuk', 'pageappadc': 'PageAppAdc',
        'pageappppm': 'PageAppPpm', 'pagemotorinfo': 'PageMotorInfo',
        'pagemotorcomparison': 'PageMotorComparison', 'experimentplot': 'ExperimentPlot',
        'detectfoc': 'DetectFoc', 'detectfocencoder': 'DetectFocEncoder',
        'detectfochall': 'DetectFocHall', 'detectbldc': 'DetectBldc',
        'detectallfocdialog': 'DetectAllFocDialog', 'nrfpair': 'NrfPair',
        'ppmmap': 'PpmMap', 'adcmap': 'AdcMap',
        'parameditdouble': 'ParamEditDouble', 'parameditint': 'ParamEditInt',
        'parameditenum': 'ParamEditEnum', 'parameditbool': 'ParamEditBool',
        'parameditstring': 'ParamEditString', 'parameditbitfield': 'ParamEditBitfield',
        'startupwizard': 'StartupWizard',
    }
    
    cls = class_map.get(basename)
    if cls is not None:
        return cls
    
    # For setupwizardapp.cpp / setupwizardmotor.cpp: multiple inner classes
    # Use context to determine which class
    return None

def guess_class(obj, method_name, method_params, filepath, context=''):
    """Guess class from object expression, method, and filepath."""
    obj = obj.strip()
    
    # this
    if obj == 'this':
        cls = guess_this_class(filepath, context)
        if cls is None:
            # Fallback for wizard files: check signal/slot names
            if method_name == 'currentIdChanged':
                return 'QWizard'
            if method_name in ('rejected', 'accepted'):
                return 'QDialog'
            if method_name in ('timerSlot', 'ended', 'nrfPairStartRes'):
                # These are slots on setupwizard inner classes - need the class
                return None
        return cls
    
    # &loop
    if obj == '&loop':
        return 'QEventLoop'
    
    # Timers
    if 'Timer' in obj or 'mTimer' in obj:
        return 'QTimer'
    
    # Serial/network
    if 'mSerialPort' in obj or 'mVictronPort' in obj:
        return 'QSerialPort'
    if 'mTcpSocket' in obj:
        return 'QTcpSocket'
    if 'mUdpSocket' in obj:
        return 'QUdpSocket'
    if 'mTcpServer' in obj or 'mTcpHubServer' in obj:
        return 'QTcpServer'
    
    # BLE
    if 'mBleUart' in obj or 'bleDevice()' in obj:
        return 'BleUart'
    if 'mService' in obj:
        return 'QLowEnergyService'
    
    # VESC types
    if 'mPacket' in obj:
        return 'Packet'
    if 'mCommands' in obj or 'commands()' in obj:
        return 'Commands'
    if any(x in obj for x in ['mMcConfig', 'mcConfig()', 'mAppConfig', 'appConfig()', 'pMc', 'pApp', 'pCustom']):
        return 'ConfigParams'
    if 'mConfig' in obj:
        return 'ConfigParams'
    if obj == 'mVesc' or (obj.startswith('mVesc') and '->' not in obj):
        return 'VescInterface'
    if obj == 'vesc' or (obj.startswith('vesc') and '->' not in obj):
        return 'VescInterface'
    
    # CAN
    if 'mCanDevice' in obj:
        return 'QCanBusDevice'
    
    # Network
    if 'reply' in obj.lower() and method_name == 'finished':
        return 'QNetworkReply'
    if 'mWebCtrl' in obj:
        return 'QNetworkAccessManager'
    if 'mOsm' in obj:
        return 'OsmClient'
    
    # Clipboard
    if 'clipboard()' in obj:
        return 'QClipboard'
    
    # Scrollbar
    if 'ScrollBar()' in obj:
        return 'QScrollBar'
    
    # Document
    if 'document()' in obj:
        return 'QTextDocument'
    
    # Process
    if 'process' in obj.lower() and method_name == 'finished':
        return 'QProcess'
    
    # Text edit
    if 'f_textedit' in obj and '->' not in obj:
        return 'QTextEdit'
    
    # Lists
    if obj in ('mCanFwdList', 'mInputList'):
        return 'QListWidget'
    if obj == 'mSensorMode':
        return 'QComboBox'
    if obj == 'mWriteButton':
        return 'QPushButton'
    
    # Boxes
    if 'mPercentageBox' in obj:
        return 'QSlider'
    if 'Box' in obj:
        if method_params == 'double':
            return 'QDoubleSpinBox'
        elif method_params == 'int':
            return 'QSpinBox'
    
    # f_* (mrichtextedit toolbar buttons)
    if obj.startswith('f_'):
        if method_name in ('clicked', 'toggled'):
            return 'QToolButton'
        if method_name == 'triggered':
            return 'QAction'
        if method_name == 'activated':
            return 'QComboBox'
        if method_name == 'setEnabled':
            return 'QWidget'
    if obj in ('removeFormat', 'removeAllFormat', 'textsource'):
        return 'QAction'
    
    # Page objects
    if 'mPageMotorSettings' in obj:
        return 'PageMotorSettings'
    if 'mPageWelcome' in obj:
        return 'PageWelcome'
    
    # qApp
    if obj == 'qApp':
        return 'QApplication'
    
    # ui-> widgets
    if obj.startswith('ui->'):
        if method_name in ('clicked', 'toggled'):
            return 'QAbstractButton'
        if method_name == 'valueChanged':
            return 'QDoubleSpinBox' if method_params == 'double' else 'QSpinBox'
        if method_name == 'currentIndexChanged':
            return 'QComboBox'
        if method_name == 'currentRowChanged':
            return 'QListWidget'
        if method_name == 'textChanged':
            return 'QLineEdit'
        if method_name == 'currentFontChanged':
            return 'QFontComboBox'
        if method_name == 'triggered':
            return 'QAction'
    
    # mBrowser
    if 'mBrowser' in obj:
        if 'ScrollBar' in obj:
            return 'QScrollBar'
        return 'QTextBrowser'
    
    return None

NEEDS_QOVERLOAD = {
    ('QSpinBox', 'valueChanged'), ('QDoubleSpinBox', 'valueChanged'),
    ('QComboBox', 'activated'), ('QComboBox', 'currentIndexChanged'),
    ('QProcess', 'finished'),
    ('QAbstractButton', 'clicked'), ('QToolButton', 'clicked'),
    ('QAction', 'triggered'), ('QPushButton', 'clicked'),
}

def build_pmf(cls, method, params):
    if (cls, method) in NEEDS_QOVERLOAD:
        if params:
            return f"qOverload<{params}>(&{cls}::{method})"
        else:
            return f"qOverload<>(&{cls}::{method})"
    return f"&{cls}::{method}"

def extract_macro(text, macro):
    idx = text.find(macro + '(')
    if idx == -1:
        return None
    start = idx + len(macro) + 1
    depth = 1
    i = start
    while i < len(text) and depth > 0:
        if text[i] == '(':
            depth += 1
        elif text[i] == ')':
            depth -= 1
        i += 1
    return text[start:i-1].strip()

def split_top_level_commas(text):
    parts = []
    depth = 0
    angle = 0
    current = ''
    for ch in text:
        if ch in '({':
            depth += 1
            current += ch
        elif ch in ')}':
            depth -= 1
            current += ch
        elif ch == '<':
            angle += 1
            current += ch
        elif ch == '>':
            if angle > 0:
                angle -= 1
            current += ch
        elif ch == ',' and depth == 0 and angle == 0:
            parts.append(current)
            current = ''
        else:
            current += ch
    if current:
        parts.append(current)
    return parts

def parse_sig(content):
    """Parse 'methodName(params)' -> (name, params)"""
    idx = content.index('(')
    return content[:idx], content[idx+1:-1].strip()

def try_convert_call(func_name, inner, filepath):
    """Try to convert connect/disconnect inner args to pointer-based."""
    
    args = split_top_level_commas(inner)
    if len(args) != 4:
        return None
    
    sender = args[0].strip()
    sig_content = extract_macro(args[1], 'SIGNAL')
    if sig_content is None:
        return None
    
    receiver = args[2].strip()
    # Remove leading whitespace/newlines from receiver
    receiver = receiver.lstrip()
    
    slot_content = extract_macro(args[3], 'SLOT')
    if slot_content is None:
        return None
    
    sig_name, sig_params = parse_sig(sig_content)
    slot_name, slot_params = parse_sig(slot_content)
    
    sender_cls = guess_class(sender, sig_name, sig_params, filepath)
    receiver_cls = guess_class(receiver, slot_name, slot_params, filepath)
    
    if sender_cls is None or receiver_cls is None:
        return None
    
    sig_pmf = build_pmf(sender_cls, sig_name, sig_params)
    slot_pmf = build_pmf(receiver_cls, slot_name, slot_params)
    
    # Detect indentation from original formatting
    if '\n' in inner:
        # Find whitespace before receiver in original
        # The receiver was args[2] which may have leading whitespace/newlines
        raw_receiver = args[2]
        indent = ''
        for ch in raw_receiver:
            if ch in ' \t\n\r':
                if ch == '\n':
                    indent = ''
                else:
                    indent += ch
            else:
                break
        new_inner = f"{sender}, {sig_pmf},\n{indent}{receiver}, {slot_pmf}"
    else:
        new_inner = f"{sender}, {sig_pmf}, {receiver}, {slot_pmf}"
    
    return f"{func_name}({new_inner})"

def process_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()
    
    result = []
    i = 0
    changes = 0
    
    while i < len(content):
        matched_func = None
        for func in ['disconnect', 'connect']:
            flen = len(func)
            if content[i:i+flen+1] == func + '(':
                # Ensure not part of another identifier
                if i > 0 and (content[i-1].isalnum() or content[i-1] == '_'):
                    continue
                matched_func = func
                break
        
        if matched_func is None:
            result.append(content[i])
            i += 1
            continue
        
        func = matched_func
        flen = len(func)
        start = i
        paren_start = i + flen  # index of '('
        
        # Find balanced closing paren
        depth = 0
        j = paren_start
        while j < len(content):
            if content[j] == '(':
                depth += 1
            elif content[j] == ')':
                depth -= 1
                if depth == 0:
                    break
            j += 1
        
        if depth != 0:
            result.append(content[i])
            i += 1
            continue
        
        inner = content[paren_start+1:j]
        
        # Skip if no SIGNAL macro or if this is a waitSignal context
        if 'SIGNAL(' not in inner:
            result.append(content[i])
            i += 1
            continue
        
        # Check context before for waitSignal
        ctx_before = content[max(0, start-30):start]
        if 'waitSignal' in ctx_before:
            result.append(content[i])
            i += 1
            continue
        
        converted = try_convert_call(func, inner, filepath)
        if converted is not None:
            result.append(converted)
            changes += 1
            i = j + 1  # skip past closing paren
        else:
            result.append(content[i])
            i += 1
    
    if changes > 0:
        new_content = ''.join(result)
        with open(filepath, 'w') as f:
            f.write(new_content)
        print(f"  {filepath}: {changes} conversions")
    
    return changes

def main():
    root = '/home/robocup/vesc_tool'
    skip_dirs = {'build', 'QCodeEditor', 'qmarkdowntextedit', 'application', '.git'}
    
    total = 0
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in skip_dirs]
        for fn in sorted(filenames):
            if not fn.endswith('.cpp') or fn == 'qcustomplot.cpp':
                continue
            fpath = os.path.join(dirpath, fn)
            with open(fpath, 'r') as f:
                c = f.read()
            if 'SIGNAL(' not in c:
                continue
            n = process_file(fpath)
            total += n
    
    print(f"\nTotal: {total} additional conversions")

if __name__ == '__main__':
    main()
