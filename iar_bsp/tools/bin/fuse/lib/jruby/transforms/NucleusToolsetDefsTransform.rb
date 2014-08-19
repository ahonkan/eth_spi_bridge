require 'BuildProperties'
require 'fileutils'
require 'rbconfig'

MAKEFILE_DEFS = <<END
# Generated -- DO NOT EDIT

END

class NucleusToolsetDefsTransform < Transform

   include BuildProperties

   def initialize(phaseList)
      super(phaseList)
      @toolset_vars = {}
      @toolset_cmds = {}
      @cmds = ["CC", "CXX", "AS", "AR", "LD", "POSTLD", "PROC_POSTLD"]
      @flag_prefixes = ["C", "CXX", "AS", "AR", "LD", "PROC_C", "PROC_CXX", "PROC_LD"]
      @tools = ["compile", "compilecxx", "assemble", "cppassemble",
      "link", "archive", "postlink",
      "proc_compile", "proc_compilecxx", "proc_assemble", "proc_cppassemble", "proc_link", "proc_postlink"]
      @includes = {}
      @libraries = {}
   end
   
   def executePhase(context)
      @context = context
      context.getUniverse().accept(self)
      # Write out toolset definitions Makefile.
      build_root = context.getBuildRoot().getAbsolutePath()
      path = File.join(build_root, outputDirectory(), "scripts", "toolset.mk")
      writeMakefile(path, self.get_def_lines())
      writeFlagFiles(build_root)
      return true
   end

   def preVisitComponent(c)
      @should_make = shouldMakeComponent?(c)
      if @should_make and not isInToolsetPackage?(c)
         @includes[c.getDirectory()] = true
      end
   end
   
   def preVisitPackage(p)
      if p.getEnabled()
        if not isInToolsetPackage?(p)
           @includes[p.getDirectory()] = true
        end
        
        if p.getName() == "nu" and p.getParent().getName() == ""
           major = p.getMajorVersionNumber();
           minor = p.getMinorVersionNumber();
           patch = p.getPatchVersionNumber();
           @toolset_vars["NUCLEUS_VERSION"] = (major * 10000 + minor * 100 + patch).to_s
        end        
      end

      if isToolsetPackage?(p)
         addGenericVariables(p)
	  elsif isSpecificToolsetPackage?(p) 
	     addPlatformSpecificVariables(p)
	  end
   end
  
   def preVisitLibrary(l)
      if @should_make
         @libraries[l.getName()] = true
      end
   end
 
   def addGenericVariables(p)
      c = p.findComponent(@context.getToolsetName())

      @cmds.each { |cmd|
         if c.hasProperty(cmd)
             @toolset_cmds[cmd] = c.getStringProperty(cmd)
         end
      }
      @flag_prefixes.each { |prefix|
         if c.hasProperty(prefix + "FLAGS")
            @toolset_vars[prefix + "FLAGS"] = c.getStringProperty(prefix + "FLAGS")
         end
      }
      @tools.each { |tool_name|
         if c.hasProperty(tool_name)
            @toolset_cmds[tool_name] = c.getStringProperty(tool_name)
         end
      }
      if c.hasProperty("CROSS")
         @toolset_vars["CROSS"] = c.getStringProperty("CROSS")
      end
      if c.hasProperty("PROC_LDSCRIPT")
         @toolset_vars["PROC_LDSCRIPT"] = c.getStringProperty("PROC_LDSCRIPT")
      end

      # Get generic process mem management and core component objects
      memmgmt_comp = @context.getUniverse().findFQComponent("nu.os.kern.process.mem_mgmt")
      core_comp = @context.getUniverse().findFQComponent("nu.os.kern.process.core")
      
      # Check if memory management generic enabled
      if memmgmt_comp.getEnabled() == true
          # Set page size equal to value configured in the core process component
          @toolset_vars["PAGE_SIZE"] = core_comp.getIntegerProperty("page_size").to_s
      else
          # Set page size to 0 when mem management not enabled
          @toolset_vars["PAGE_SIZE"] = "0"
      end

   end
   
   def addPlatformSpecificVariables(p)
      c = p.findComponent(@context.getToolsetName())
      @flag_prefixes.each { |prefix|
         prop_name = "ARCH_" + prefix + "FLAGS"
         if (c.hasProperty(prop_name))
             @toolset_vars[prop_name] = c.getStringProperty(prop_name)
         end
      }
      @toolset_vars["ARCH_LDSCRIPT"] = c.getStringProperty("ARCH_LDSCRIPT")
   end

   def writeMakefile(makefile_path, lines)
      makefile = File.new(makefile_path, "w")
      template = fixupLines(lines)
      template.each { |line| makefile.write(line) }
      makefile.close
   end
  
   def writeFlagFiles(build_root)
      system_includes = []

      # Get system include paths from global property
      system_include_property = @context.getUniverse().getProperty("system_include_paths")
      system_include_array = system_include_property.split(",")
      
      # Add system include paths
      system_include_array.each { |p| system_includes << "-isystem " + build_root.gsub("\\","/") + "/" + p.gsub("\\","/")}
      includes = system_includes.join(" ")
            
      # Add include paths
      includes << @includes.keys.map { |p| "-I" + p.to_s if p }.join(" ").gsub("\\", "/")

      lib_root = File.join(build_root, outputDirectory(), "lib")

      getflags = lambda { |flag_set_name, use_includes|
         flags = ""
         flags += @toolset_vars[flag_set_name + "FLAGS"] + " "
         flags += @toolset_vars["ARCH_" + flag_set_name + "FLAGS"] + " "
         if use_includes
            flags += includes
         end
         flags
      }

      # Ensure that the output directory exist.
      if not File.exists?(lib_root)
         FileUtils.mkdir_p(lib_root)
      end

      @libraries.keys.each { |library_name|
         # Write compiler and assembler options
         ["C", "CXX", "AS"].each { |flag_set_name|
            filename = library_name + "." + flag_set_name.downcase + "flags"
            File.open(File.join(lib_root, filename), "w") { |file| 
               file.write(getflags.call(flag_set_name, true)) 
            }
         }

         # Write linker options.
         filename = library_name + ".ldflags"
         toolsets_path = File.join(build_root, "bsp", @context.getPlatformName(), "toolset")
         toolsets_path.gsub!(/\\/, "/")		    
         File.open(File.join(lib_root, filename), "w") { |file| 
            flags = getflags.call("LD", false)
            # Expand $(TOOLSETS_HOME)
            flags.gsub!("$(TOOLSETS_HOME)", toolsets_path)
            file.write(flags) 
         }
      }
   end
 
   def fixupLines(lines)
      if Config::CONFIG['host_os'] == 'mswin32'
         lines.gsub!(/\n/,"\r\n")
      end
      lines
   end

   def get_def_lines
      lines = MAKEFILE_DEFS
      @toolset_vars.each { |name, value|
         if name == "PAGE_SIZE"
            lines += name + " = " + value + "\n"
         else
            lines += name + " += " + value + "\n"
         end
      }
      @toolset_cmds.each { |name, value|
         lines += name + " = " + value + "\n"
      }
      lines
   end
end
