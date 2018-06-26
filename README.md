Timelapse View for HG
=====================

![preview](https://user-images.githubusercontent.com/4054655/41914971-95a10d40-7922-11e8-95aa-e709832b8b5a.gif)

# Features

- Show all file revisions in a time-browsable view (designed for [Unity](https://unity3d.com/) mercurial repos)

  ![main view](https://user-images.githubusercontent.com/4054655/41929293-9d727810-7945-11e8-883b-74a848dd52bb.png)
  
- Base all file revision on a common ancestor and sort them out intelligibly. 

  ![timelapse_scrub_time](https://user-images.githubusercontent.com/4054655/41929432-fc22fd4e-7945-11e8-8f44-248193037f9e.gif)
   
- Show common and useful information:

  ![rev info](https://user-images.githubusercontent.com/4054655/41929826-5218af72-7947-11e8-99b3-a1a7aead9369.png)  
  
- You want to view the `diff` patch instead?

  ![image](https://user-images.githubusercontent.com/4054655/41929893-83c7e9d4-7947-11e8-9fac-268789c22014.png)
  ![image](https://user-images.githubusercontent.com/4054655/41929921-9747850a-7947-11e8-97d6-2f0fc63c5256.png)
  
- You can use keyboard shortcuts for quick navigation through revisions:

  ![image](https://user-images.githubusercontent.com/4054655/41929955-af229624-7947-11e8-8c39-d91748a9e82d.png)
  
- Standalone and lightweight executable (less than **1 mb**)

  ![image](https://user-images.githubusercontent.com/4054655/41930047-ecabec8e-7947-11e8-9272-2962de508ba8.png)
  
- Reuse the same instance to timelapsed other repo files:

  ![image](https://user-images.githubusercontent.com/4054655/41930160-384b9702-7948-11e8-9bd7-b8de4cb43d43.png)
  
- Rebasing all revisions on a common ancestor can be a long process (`hg` is slow as _hell_!) so we let the user know how is the annotations fetching and revision rebasing progressing:

  ![timelapse_taskbar_progress](https://user-images.githubusercontent.com/4054655/41930281-9d1e40a8-7948-11e8-9b7f-d422a828e24d.gif)
  
- Support various external tool to work with the current revision; such as opening beycond compare as a folder compare view or opening various `thg` tools:

  ![image](https://user-images.githubusercontent.com/4054655/41930374-df55c8c4-7948-11e8-805d-915a6502c49a.png)
  
- Show the file change set using a simple lis view

  ![timelapse_file_changeset](https://user-images.githubusercontent.com/4054655/41935995-b3c400e8-7959-11e8-9a01-b324d6487212.gif)
  
- Launching multiple `hg` process in background thread to keep the UI responsive.

# Download

You can download the latest pre-built version:

- **Windows**: <https://github.com/jschmidt42/timelapse/raw/master/release/win64/timelapse.exe>

# Using

- ocornut/imgui <https://github.com/ocornut/imgui> for the UI
- rampantpixels/foundation_lib <https://github.com/rampantpixels/foundation_lib> for the common platform code
