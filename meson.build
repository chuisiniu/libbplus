project('libbplus', 'c', default_options: ['warning_level=3', 'werror=true'])

add_global_arguments('-Wno-unused-parameter', language : 'c')
add_global_arguments('-Wno-pedantic',         language : 'c')
add_global_arguments('-Wno-pedantic',         language : 'cpp')

libbplus_src = ['bplus.c']

libbplus = library('bplush', libbplus_src)

gtest_dep = dependency('gtest', main : true, required : false)
if gtest_dep.found()
   add_languages('cpp')

   test_src = ['test.cc']
   e = executable('testprog', test_src, dependencies : gtest_dep,
                  link_with: libbplus)

   test('gtest test', e)
endif