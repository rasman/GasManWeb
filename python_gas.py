import ctypes
import os
from pathlib import Path
import platform
import xml.etree.ElementTree as ET
import pandas as pd
import json
from datetime import time

def xml_to_dict(element):
    result = {"tagName": element.tag}
    for key, value in element.attrib.items():
        result[key] = value
    if element.text and element.text.strip():
        result["text"] = element.text.strip()
    for child in element:
        child_result = xml_to_dict(child)
        if child.tag not in result:
            result[child.tag] = child_result
        else:
            if not isinstance(result[child.tag], list):
                result[child.tag] = [result[child.tag]]
            result[child.tag].append(child_result)
    return result

def convert_xml_to_json(xml_file):
    tree = ET.parse(xml_file)
    root = tree.getroot()
    return xml_to_dict(root)

def ensure_tag_name(data, default_tag="gasman"):
    if isinstance(data, dict):
        # Ensure 'tagName' exists in each dictionary
        data.setdefault("tagName", default_tag)
        for key, value in data.items():
            ensure_tag_name(value)  # Recursively check nested dictionaries/lists
    elif isinstance(data, list):
        for item in data:
            ensure_tag_name(item)

def serialize_data_with_tags(data):
    # Ensure tag names exist before serializing
    ensure_tag_name(data)
    return serialize_data(data)  # Existing serialize_data logic

def process_file(file_path, output_file=None):
    _, file_extension = os.path.splitext(file_path)

    if file_extension.lower() == '.json':
        with open(file_path) as f:
            return json.load(f)
    elif file_extension.lower() == '.xml':
        data = convert_xml_to_json(file_path)
        print("XML data processed into memory.")
        return data
    elif file_extension.lower() == '.xlsx':
        params_df, settings_df = read_excel_data(file_path)
        params_dict = params_df.to_dict(orient='records')
        settings_dict = settings_df.to_dict(orient='records')

        # Add default tagName
        data = {
            "tagName": "gasman",
            "params": {"tagName": "params", **{"data": params_dict}},
            "settings": {"tagName": "settings", **{"data": settings_dict}}
        }
        return data
    
    else:
        print(f"Unsupported file type: {file_extension}")
        return None

def read_excel_data(excel_file):
    params_df = pd.read_excel(excel_file, sheet_name='Parameters')
    settings_df = pd.read_excel(excel_file, sheet_name='Settings')
    return params_df, settings_df

def serialize_data(data):
    # Recursive function to convert `time` objects to strings
    if isinstance(data, dict):
        return {key: serialize_data(value) for key, value in data.items()}
    elif isinstance(data, list):
        return [serialize_data(item) for item in data]
    elif isinstance(data, time):
        return data.strftime("%H:%M:%S")  # Convert to string (HH:MM:SS format)
    else:
        return data

def load_shared_library(lib_name):
    system = platform.system()
    ext_map = {'Windows': '.dll', 'Darwin': '.dylib'}
    lib_ext = ext_map.get(system, '.so')
    lib_dir = 'x64\Release' if system == 'Windows' else 'compiled'

    lib_path = Path(__file__).resolve().parent / lib_dir / f"{lib_name}{lib_ext}"
    print(f"Resolved path: {lib_path}")

    if not lib_path.is_file():
        raise FileNotFoundError(f"Cannot find shared library at {lib_path}")
    return ctypes.CDLL(str(lib_path))

def process_with_library(data, output_file=None):
    gas_lib = load_shared_library("CPPDLL")
    gas_lib.GasManJsonToCsv.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int]
    gas_lib.GasManJsonToCsv.restype = ctypes.c_char_p

    # Serialize data to make it JSON-compatible
    
    serialized_data = serialize_data(data)
    json_data = json.dumps(serialized_data).encode('utf-8')

    start_second = 0
    end_second = 300
    every_seconds = 10

    try:
        csv_result = gas_lib.GasManJsonToCsv(json_data, len(json_data), start_second, end_second, every_seconds).decode('utf-8')

        if not output_file:
            output_file = 'output.csv'
        with open(output_file, 'w') as f:
            f.write(csv_result)
        print(f"CSV output saved to {output_file}")
    except Exception as e:
        print(f"Error during library call: {e}")

def main(file_path, output_file=None):
    data = process_file(file_path, output_file)
    if data:
        if 'imageData' in data:
            del data['imageData']
        process_with_library(data, output_file)

if __name__ == "__main__":
    import sys
    if len(sys.argv) not in [2, 3]:
        print("Usage: python python_gas.py <input_file> [output_file]")
    else:
        file_path = sys.argv[1]
        output_file = sys.argv[2] if len(sys.argv) == 3 else None
        main(file_path, output_file)
