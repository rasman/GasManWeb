import xml.etree.ElementTree as ET
import json

def xml_to_dict(element):
    result = {}
    for child in element:
        if len(child):
            result[child.tag] = xml_to_dict(child)
        else:
            result[child.tag] = child.text
    return result

def convert_xml_to_json(xml_file):
    tree = ET.parse(xml_file)
    root = tree.getroot()
    xml_dict = xml_to_dict(root)
    json_str = json.dumps(xml_dict, indent=4)
    return json_str

def save_json_to_file(json_str, json_file):
    with open(json_file, 'w') as f:
        f.write(json_str)
    print(f"Converted to {json_file}")

def main(xml_file, json_file):
    json_str = convert_xml_to_json(xml_file)
    save_json_to_file(json_str, json_file)

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 3:
        print("Usage: python xml_to_json_converter.py <input_xml_file> <output_json_file>")
    else:
        xml_file = sys.argv[1]
        json_file = sys.argv[2]
        main(xml_file, json_file)
