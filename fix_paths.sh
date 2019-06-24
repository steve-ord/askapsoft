#!/usr/bin/env bash 

h_files=`find . -name "*.h"`
c_files=`find . -name "*.cc"`
t_files=`find . -name "*.tcc"`

all_files="$h_files $c_files $t_files"

for file in $all_files
do	
  # fix namespaces
  sed -i '' 's|'\ casa\;'|'\ casacore\;'|g' $file
  sed -i '' 's|'casa\:'|'casacore\:'|g' $file

  #include paths	
  sed -i '' 's|'include\ \<askap/'|'include\ \<askap/askap/'|g' $file
  sed -i '' 's|'include\ \"askap/'|'include\ \"askap/askap/'|g' $file
  sed -i '' 's|'include\ \<askapparallel/'|'include\ \<askap/askapparallel/'|g' $file
  sed -i '' 's|'include\ \"askapparallel/'|'include\ \"askap/askapparallel/'|g' $file

  sed -i '' 's|'include\ \<utils/'|'include\ \<askap/scimath/utils/'|g' $file
  sed -i '' 's|'include\ \"utils/'|'include\ \"askap/scimath/utils/'|g' $file
  sed -i '' 's|'include\ \<scimath/'|'include\ \<askap/scimath/'|g' $file
  sed -i '' 's|'include\ \"scimath/'|'include\ \"askap/scimath/'|g' $file

  sed -i '' 's|'include\ \<components/AskapComponentImager.h'|'include\ \<askap/components/AskapComponentImager.h'|g' $file
  sed -i '' 's|'include\ \"components/AskapComponentImager.h'|'include\ \"askap/components/AskapComponentImager.h'|g' $file

#  sed -i '' 's|'include\ \<components/ComponentModels/'|'include\ \<casarest/components/ComponentModels/'|g' $file
#  sed -i '' 's|'include\ \"components/ComponentModels/'|'include\ \"casarest/components/ComponentModels/'|g' $file
done
