require 'BuildProperties'
require 'fileutils'
require 'rbconfig'

MAKEFILE_LIBS = <<END
%s: %s

END

MAKEFILE_EXES = <<END
%s: %s %s

END

MAKEFILE_PROCS = <<END
%s: %s %s

END

MAKEFILE_OBJS = <<END
# Generated -- DO NOT EDIT

OBJS = %s
PROC_OBJS = %s

END

MAKEFILE_METADATA = <<END
# Generated -- DO NOT EDIT

METADATA = %s

END

MAKEFILE_COMMON = <<END
# Generated -- DO NOT EDIT

CURRENT_TOOLSET = %s
CURRENT_PLATFORM = %s
CURRENT_CONFIG = %s
CURRENT_USER_CONFIG = %s

END

MAKEFILE_COMMON_SPECIFIC = <<END
# Generated -- DO NOT EDIT

include %s/toolset.mk

CURRENT_TOOLSET = %s
CURRENT_PLATFORM = %s
CURRENT_CONFIG = %s
CURRENT_USER_CONFIG = %s
INCLUDE_PATH = %s
SYSTEM_INCLUDE_PATH = %s
TOOLSETS_HOME = "%s"
CONFIG_FILES = %s

END

class NucleusMakefileTransform < Transform

   include BuildProperties

   def initialize(phaseList)
      super(phaseList)
      @build_root = nil
      @objs = []
      @objs_flat = []
      @flags = []
      @ldscript = ""
      @sub_dirs = []
      @exe_name = nil
      @proc_name = nil
      @lib_name = nil
      @include_paths = []
      @system_include_paths = []
      @lib_names = []
      @should_make = false
      @universe = nil
      @objs_all = []
      @proc_objs_all = []
      @exes_all = []
      @procs_all = []
      @libs_all = []
      @scripts_path_abs = nil
   end
   
   def executePhase(context)
      @build_root = context.getBuildRoot()
      @context = context
      @universe = context.getUniverse()

      build_root = @build_root.getAbsolutePath()
      scripts_path = File.join(outputDirectory(), "scripts")
      @scripts_path_abs = File.join(build_root, scripts_path)

      # Ensure that the scripts output directory exist.
      if not File.exists?(@scripts_path_abs)
         FileUtils.mkdir_p(@scripts_path_abs)
      end

      # Create empty flags.mk, libs.mk, exes.mk and procs.mk files and put
      # warning header comment in them
      makefile_path = File.join(@scripts_path_abs, "flags.mk")
      writeAppendMakefile(makefile_path, "# Generated -- DO NOT EDIT\n\n")
      makefile_path = File.join(@scripts_path_abs, "libs.mk")
      writeAppendMakefile(makefile_path, "# Generated -- DO NOT EDIT\n\n")
      makefile_path = File.join(@scripts_path_abs, "exes.mk")
      writeAppendMakefile(makefile_path, "# Generated -- DO NOT EDIT\n\n")
      makefile_path = File.join(@scripts_path_abs, "procs.mk")
      writeAppendMakefile(makefile_path, "# Generated -- DO NOT EDIT\n\n")

      context.getUniverse().accept(self)

      # Insert include path of generated header file
      @include_paths.unshift(File.join(outputDirectory(), "gen", "include"))     

      # Remove nil entries from included paths
      @include_paths.compact!

      # Adding this property so we can access it elsewhere
      @universe.setProperty("include_path", @include_paths.join(','))
      
      # Remove nil entries from system included paths
      @system_include_paths.compact!

      # Adding this property so we can access it elsewhere
      @universe.setProperty("system_include_paths", @system_include_paths.join(','))      
      
      # Write out common Makefile
      common_path = File.join(build_root, "tools", "scripts", "make", "Makefile.common")
      common_vars = [@universe.getProperty("toolset"),
                     @universe.getProperty("platform"),
                     @universe.getProperty("config"),
                     @universe.getProperty("userconfig")]
      writeMakefile(common_path, MAKEFILE_COMMON % common_vars)
      
      # Write out configuration specific common Makefile - common.mk
      toolsets_path = File.join("$(SYSTEM_HOME)", "bsp", context.getPlatformName(), "toolset")
      
      # Set initial common makefile values to global variable values
      common_vars = [scripts_path,
                     @universe.getProperty("toolset"),
                     @universe.getProperty("platform"),
                     @universe.getProperty("config"),
                     @universe.getProperty("userconfig"),      
                     @include_paths.join(' '),
                     @system_include_paths.join(' '), toolsets_path]

      # Build up a list of the used configuration files.
      config_files = ""
      context.cliProperties.each { |key,value|
         key = key.sub(/\.\*[0-9]+\*/,'') if key.start_with?('.*')               
         if key.start_with?(".include")
            config_files += " #{value}"
         end
      }
      common_vars << config_files
      common_path = File.join(@scripts_path_abs, "common.mk")
      writeMakefile(common_path, MAKEFILE_COMMON_SPECIFIC % common_vars)
      
      # Write out list of metadata files that are part of the applied configuration.
      metadata_list = ""
      @sub_dirs.each { |sub_dir|
          metadata_list += File.join(sub_dir, ".metadata") + " "
      }
      metadata_list.chop
      writeMakefile(File.join(@scripts_path_abs, "metadata.mk"), 
          MAKEFILE_METADATA % metadata_list)

      # Insert the generated source objects in the objects list. 
      gen_objects = "$(patsubst %.c,$(OBJ_DIR)/%.o,$(wildcard " + File.join(outputDirectory(), "gen", "src", "*.c") + "))"
      @objs_all << gen_objects 

      # Write out list of objects.
      obj_list = [@objs_all.join(' ')]
      proc_obj_list = [@proc_objs_all.join(' ')]
      template_params = []
      template_params << obj_list
      template_params << proc_obj_list
      writeMakefile(File.join(@scripts_path_abs, "objs.mk"), 
                    MAKEFILE_OBJS % template_params)

      # Write out the list of libraries to be built. Note that the rules have
      # already been written. Here we just add one additional rule for the generated
      # source objects.
      libs_vars = ["$(LIB_OUTPUT_DIR)/nucleus.lib", gen_objects]
      writeAppendMakefile(File.join(@scripts_path_abs, "libs.mk"), MAKEFILE_LIBS % libs_vars)
      lib_list = ""
      @libs_all.each { |lib_name|
          lib_list += "$(LIB_OUTPUT_DIR)/" + lib_name + " "
      }
      lib_list.chop
      writeAppendMakefile(File.join(@scripts_path_abs, "libs.mk"),
         "# List of the libraries to be built\n" + 
         "LIBS = " + lib_list) 

      # Write out the list of executables to be generated. The rules have already been written.
      exe_list = ""
      @exes_all.each { |exe_name|
          exe_list += "$(EXE_OUTPUT_DIR)/" + exe_name + " "
      }
      exe_list.chop
      writeAppendMakefile(File.join(@scripts_path_abs, "exes.mk"),
         "# List of the executables to be built\n" + 
         "EXES = " + exe_list) 

      # Write out the list of processes to be generated. The rules have already been written.
      proc_list = ""
      @procs_all.each { |proc_name|
          proc_list += "$(PROC_OUTPUT_DIR)/" + proc_name + " "
          
          # Add the dependency to "nucleus_user.lib" as all Nucleus processes need it.
          writeAppendMakefile(File.join(@scripts_path_abs, "procs.mk"),
              "$(PROC_OUTPUT_DIR)/" + proc_name + ": $(LIB_OUTPUT_DIR)/nucleus_user.lib\n\n")
          writeAppendMakefile(File.join(@scripts_path_abs, proc_name + "_libs.mk"),
              outputDirectory() + "/lib/nucleus_user.lib\n\n")
      }
      proc_list.chop
      writeAppendMakefile(File.join(@scripts_path_abs, "procs.mk"),
         "# List of the processes to be built\n" + 
         "PROCS = " + proc_list) 
      
      # Write out config file.
      return writeConfigFile()
   end
   
   def preVisitPackage(p)
      if p.getEnabled() and not isInToolsetPackage?(p) \
           and autoIncludePathEnabled?()
         @include_paths << p.getDirectory()
      end
   end   
      
   def preVisitComponent(c)
      @base_directory = c.getDirectory();
      fixupped_dir = fixupFileName(@context.getBuildRoot(), @base_directory)
      if not @sub_dirs.include?(fixupped_dir)
         @sub_dirs << fixupped_dir
      end
      @should_make = shouldMakeComponent?(c)
      if @should_make
         if not isInToolsetPackage?(c) and autoIncludePathEnabled?()
            @include_paths << c.getDirectory()
         end
         if c.hasProperty("includepath")
            includepath = c.getStringProperty("includepath")
            includepath.split(":").each { |path|
               @include_paths << path
            }
         end

         # Add system include paths to global system include paths array
         if c.hasProperty("systemincludepath")
            systemincludepath = c.getStringProperty("systemincludepath")
            systemincludepath.split(":").each { |path|
               @system_include_paths << path                
            }
         end
      end
   end
   
   def postVisitComponent(c)
      flags_makefile_path = File.join(@scripts_path_abs, "flags.mk")
      template_params = []
      if @should_make
         active_artifact = ""
         artifact_processed = false
         if c.hasProperty("active_artifact")
            active_artifact = c.getStringProperty("active_artifact")
         end 
         if ((active_artifact.empty? or active_artifact == "library") and @lib_name != nil)
            if active_artifact.empty?
                c.setStringProperty("active_artifact", "library")
            end
            makefile_path = File.join(@scripts_path_abs, "libs.mk")
            template_params << "$(LIB_OUTPUT_DIR)/" + @lib_name
            template_params << @objs.join(' ')
            writeAppendMakefile(makefile_path, MAKEFILE_LIBS % template_params)
            if not @flags.empty?
               writeAppendMakefile(flags_makefile_path, @flags.join("\n") + "\n\n")
            end
            if not @libs_all.include? @lib_name
               @libs_all << @lib_name
            end
            artifact_processed = true
         end
         if ((!artifact_processed and (active_artifact.empty? or active_artifact == "executable")) and @exe_name != nil)
            if active_artifact.empty?
                c.setStringProperty("active_artifact", "executable")
            end
            makefile_path = File.join(@scripts_path_abs, "exes.mk")
            template_params << "$(EXE_OUTPUT_DIR)/" + @exe_name
            template_params << @objs.join(' ')
            lib_list = ""
            @lib_names.each { |lib_name|
               lib_list += "$(LIB_OUTPUT_DIR)/" + lib_name + " "
            }
            lib_list.chop
            template_params << lib_list
            writeAppendMakefile(makefile_path, MAKEFILE_EXES % template_params)
            if not @ldscript.empty?
               writeAppendMakefile(makefile_path,
                  "# Linker script for " + @exe_name + "\n" +
                  @ldscript + "\n\n")
            end
            if not @flags.empty?
               writeAppendMakefile(flags_makefile_path, @flags.join("\n") + "\n\n")
            end
            if not @exes_all.include? @exe_name
               @exes_all << @exe_name
            end
            artifact_processed = true
         end   
         if ((!artifact_processed and (active_artifact.empty? or active_artifact == "process")) and @proc_name != nil)
            if active_artifact.empty?
                c.setStringProperty("active_artifact", "process")
            end
            makefile_path = File.join(@scripts_path_abs, "procs.mk")
            template_params << "$(PROC_OUTPUT_DIR)/" + @proc_name
            template_params << @objs.join(' ')
            lib_list = ""
            lib_list_flat = ""
            @lib_names.each { |lib_name|
               lib_list += "$(LIB_OUTPUT_DIR)/" + lib_name + " "
               lib_list_flat += outputDirectory() + "/lib/" + lib_name + "\n"
            }
            lib_list.chop
            template_params << lib_list
            writeAppendMakefile(makefile_path, MAKEFILE_PROCS % template_params)
            writeAppendMakefile(File.join(@scripts_path_abs, @proc_name + "_objs.mk"), @objs_flat.join("\n") + "\n")
            writeAppendMakefile(File.join(@scripts_path_abs, @proc_name + "_libs.mk"), lib_list_flat)
            if not @flags.empty?
               writeAppendMakefile(flags_makefile_path, @flags.join("\n") + "\n\n")
            end
            if not @procs_all.include? @proc_name
               @procs_all << @proc_name
            end
         end
      end
      @lib_name = nil
      @exe_name = nil
      @proc_name = nil      
      @objs = []
      @objs_flat = []
      @lib_names = []
      @flags = []
      @ldscript = ""
   end
   
   def postVisitExecutable(e)
      @exe_name = e.getName()
      if (@should_make and (!e.hasProperty("active_artifact") or e.getStringProperty("active_artifact") == "executable"))
         e.getSourceFiles().each { |file|
            fixedupFileName = fixupFileSuffix(fixupFileName(@build_root, file), 'o')
            @objs << "$(OBJ_DIR)/" + fixedupFileName
            @objs_all << "$(OBJ_DIR)/" + fixedupFileName
            appendFlags(e, file)
         }
         e.getLibraries().each { |lib_name|
            @lib_names << lib_name.getName()
         }
         appendFlags(e, Java::JavaIo::File.new(@exe_name))
         updateLinkerScript(e, Java::JavaIo::File.new(@exe_name))
      end
   end

   def postVisitProcess(p)
      @proc_name = p.getName()
      if (@should_make and (!p.hasProperty("active_artifact") or p.getStringProperty("active_artifact") == "process"))
         p.getSourceFiles().each { |file|
            fixedupFileName = fixupFileSuffix(fixupFileName(@build_root, file), 'po')
            @objs << "$(OBJ_DIR)/" + fixedupFileName
            @objs_flat << outputDirectory() + "/objs/" + fixedupFileName
            @proc_objs_all << "$(OBJ_DIR)/" + fixedupFileName
            appendFlags(p, file)
         }
         p.getLibraries().each { |lib_name|
            @lib_names << lib_name.getName()
         }
         appendFlags(p, Java::JavaIo::File.new(@proc_name))
      end
   end
   
   def postVisitLibrary(l)
      @lib_name = l.getName()
      if (@should_make and (!l.hasProperty("active_artifact") or l.getStringProperty("active_artifact") == "library"))
         l.getSourceFiles().each { |file|
            fixedupFileName = fixupFileSuffix(fixupFileName(@build_root, file), 'o')
            @objs << "$(OBJ_DIR)/" + fixedupFileName
            @objs_all << "$(OBJ_DIR)/" + fixedupFileName
            appendFlags(l, file)
         }
      end
   end
   
   def fixupFileName(base_directory, file)
      file_name = file.getAbsolutePath()
      file_name.sub!(base_directory.getAbsolutePath(), '')
      # previous sub may leave a leading '/'or '\', if so axe it.
      file_name.sub!(/^[\\\/]/, '')
      # sanitize slashes
      file_name.gsub!(/\\/, "/")
      return file_name
   end
  
   def fixupFileSuffix(file_name, suffix)
       file_name.sub!(/(.+)\..+/, '\1.' + suffix)
      return file_name
   end

   def appendFlags(obj, file)
      toolset_name = @context.getToolsetName()

      ["cflags", "cxxflags", "asflags", "ldflags"].each { |flag_type|
         flag_property_name = toolset_name + "." + flag_type
         flag_property_name += "." + file.getName()

         if obj.hasProperty(flag_property_name) \
               and obj.getProperty(flag_property_name) != nil
            value = flag_type.upcase + "_"
            value += fixupFileName(@build_root, file)
            value += " = " + obj.getProperty(flag_property_name)
            @flags << value
         end
      }
   end

   def updateLinkerScript(obj, file)
      toolset_name = @context.getToolsetName()
      ldscript_property_name = "#{toolset_name}.ldscript"

      # If the user has provided their own linker script, then
      # use that.
      if obj.hasProperty(ldscript_property_name) \
            and obj.getProperty(ldscript_property_name) != nil
         ldscript_path = obj.getProperty(ldscript_property_name)
      else
         # Otherwise, use the default script.
         ldscript_path = "$(ARCH_LDSCRIPT)"
      end

      @ldscript = "LDSCRIPT_#{fixupFileName(@build_root, file)}"
      @ldscript += " = #{ldscript_path}"
   end

   def writeMakefile(makefile_path, lines)
      makefile = File.new(makefile_path, "w")
      template = fixupLines(lines)
      template.each { |line| makefile.write(line) }
      makefile.close
   end

   def writeAppendMakefile(makefile_path, lines)
       # Create new file if it doesn't already exist.
      if not File.exists?(makefile_path)
          makefile = File.new(makefile_path, "w")
      else                                                                                                            
          makefile = File.open(makefile_path, "a")          
      end
      template = fixupLines(lines)
      template.each { |line| makefile.write(line) }
      makefile.close
   end
   
   def writeConfigFile()
      abs_output_dir = File.join(@build_root.getAbsolutePath(), 
                                 outputDirectory())

      # Ensure that the output directory exist.
      if not File.exists?(abs_output_dir)
         FileUtils.mkdir_p(abs_output_dir)
      end

      config_file = File.join(outputDirectory(), "current.imageconfig")
      config_writer = ConfigStateWriterTransform.new(nil, config_file)
      return config_writer.executePhase(@context)      
   end

   def fixupLines(lines)
      if Config::CONFIG['host_os'] == 'mswin32'
         lines.gsub!(/\n/,"\r\n")
      end
      lines
   end

   def autoIncludePathEnabled?()
       return @universe.hasProperty("autoincludepath") \
              && (@universe.getProperty("autoincludepath") == "1")
   end
end

