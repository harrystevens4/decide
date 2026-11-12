
# About

this is a desktop portal for file selection (please consult flowchart to see if this matches your use case)
- terminal based file selector
- you know when you are already at the file in the terminal but now you have to use a gui to find and then upload said file? no longer a problem
- running `decide /tmp/Makefile.txt` in terminal selects file
- thats it

```
   .----------------.
  |                  |
  |      start       |
  |                  |
   '----------------'

           |
           |
           v

           ^
          / \
         /   \
        / do  \
       / you   \
      /  need   \
     / something \            yes
    |             | ------------------------. 
     \   that    /                           |
      \  works? /                            |
       \       /                             |
        \     /                              |
         \   /                               |
          \ /                                |
           v                                 |
                                             |
           |                                 |
           | no                              |
           v                                 |
                                             |
           ^                                 |
          / \                                |
         /   \                               |
        /     \                              |
       /       \                             |
      /   do    \                            |
     /   you     \         yes               |
    |    like     | ------------------------>|
     \   guis?   /                           |
      \         /                            |
       \       /                             |
        \     /                              |
         \   /                               |
          \ /                                |
           v                                 |
                                             |
           |                                 |
           | no                              |
           v                                 |
                                             |
+---------------------+                      |
|                     |                      |
|  use this project   |                      |
|                     |                      |
+---------------------+                      |
                                             |
           |                                 |
           |                                 |
           v                                 |
                                             v
   .----------------.          +-------------------------+
  |                  |         |                         |
  |       end        | <------ |  dont use this project  |
  |                  |         |                         |
   '----------------'          +-------------------------+
                                                
```                                             
