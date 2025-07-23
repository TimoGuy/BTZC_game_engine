import argparse
from unityparser import UnityDocument

# Parse args to program.
parser = argparse.ArgumentParser(prog='Scene importer',
                                 description='Converts a Unity scene to a BTZC scene.')
parser.add_argument('unity_scene_fname')
parser.add_argument('btzc_scene_fname')

args = parser.parse_args()



if __name__ == '__main__':
    try:
        unity_doc = UnityDocument.load_yaml(args.unity_scene_fname)
        pass
    except FileNotFoundError:
        print(f'ERROR: {args.unity_scene_fname} not found.')
