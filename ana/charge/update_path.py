"""add the pflib/ana directory to the sys.path so its modules can be found

e.g.

    from read import read_pflib_csv

works after updating the path like this.
"""

import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent.parent))
