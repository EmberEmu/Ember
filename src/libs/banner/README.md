This target only serves to prevent recompilation cascades triggered by actions that update the Git hash embedded within the source.

Previously, the Git version information was contained within the shared library. Here's what would happen:
- git commit
- Version.cpp changed
- 'shared' library changed, rebuild
- code generation targets relink to 'shared'
- code generation reruns since the generators have changed
- targets that depend on code generation output rebuilt

Extracting Version.cpp and not including it in code generation targets prevents this and makes life more pleasant.