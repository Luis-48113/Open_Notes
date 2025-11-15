Open Notes 📝

A modern, sleek notes application for Linux with a GTK3 GUI. Inspired by popular note apps, Open Notes lets you create, view, edit, and delete notes with a clean interface. Perfect for keeping your thoughts organized.

Features

Modern, user-friendly GTK3 interface

Create, edit, and delete notes

Notes saved locally in a dedicated folder

Preview area to quickly see your notes

Cross-platform compatible with C source (Linux/macOS)

Installation
Using the .deb Package (Linux only)

Download the latest .deb from Releases
 or your repo.

Open a terminal and navigate to the folder containing the .deb.

Install with:

sudo dpkg -i open-notes-deb.deb


Run the app:

notes_app


Your notes will be stored in a folder named notes in the same directory.

Compiling from Source (Linux/macOS)

Clone the repo:

git clone https://github.com/yourusername/open-notes.git
cd open-notes


Ensure you have the required dependencies:

Linux (Ubuntu/Debian)

sudo apt update
sudo apt install gcc pkg-config libgtk-3-dev


macOS (using Homebrew)

brew install gtk+3


Compile the C source:

gcc `pkg-config --cflags gtk+-3.0` note_app.c -o notes_app `pkg-config --libs gtk+-3.0`


Run the app:

./notes_app

Usage

Enter a note title and content, then click Save Note.

Click Open on a note in the sidebar to view or edit it.

Click Delete to remove a note permanently.

Notes are stored locally in the notes folder.

License:

Apache 2.0 License © 2025 Luis-48113
