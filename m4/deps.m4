AC_DEFUN([RK_DEPENDENCY_DEF], [
    #DEPENDENCY: $1
    PKG_CHECK_MODULES(patsubst(m4_toupper(m4_normalize(patsubst($1,[[<=> ]+.*], [ ]))), [[^0-9a-zA-Z]],[_]), $1)
])

AC_DEFUN([RK_CHECK_MODULES], [
    define(rk_project_depsline, m4_esyscmd_s([cat project.properties | grep "^[ \t]*DEPENDENCIES[ \t=]" | grep -v "^[ \t]*#" | cut -d"=" -f2-]))
    define(rk_project_dependencies, m4_split(rk_project_depsline,[ *; *]))
    m4_foreach([dep], [rk_project_dependencies], [RK_DEPENDENCY_DEF(m4_normalize(dep))])
])

