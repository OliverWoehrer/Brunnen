from tkinter import filedialog
import os

# Select Input Files:
file_paths = filedialog.askopenfilenames(filetypes=[("CSV Files", "*.txt")])

# Read XML Files and Find Elements:
for file_path in file_paths:
    splits = file_path.split("/")
    file_name = splits[-1]
    if file_name.startswith("-"):
        new_file_name = file_name.replace("-", "", 1)
        new_file_path = "/".join(splits[:-1]) + "/" + new_file_name

    # Renaming the file
    os.rename(file_path, new_file_path)
    print("renamed",new_file_path)