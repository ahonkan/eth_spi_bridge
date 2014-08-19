require 'BuildProperties'
require 'fileutils'
require 'rbconfig'

class BuildPropertiesWriterTransform < Transform

   include BuildProperties

   def initialize(phaseList, conf_filename="system.properties")
      super(phaseList)
      @settings = {}
      @namespace = []
      @output = ""
      @conf_filename = conf_filename
      @toolset_vals = {}
      @abs_includes = ""
      @abs_includes_generic = ""
      @abs_system_includes = ""
      @abs_system_includes_generic = ""
      @oflags = {}
   end
   
   def executePhase(context)
      @context = context
      @toolset = @context.getToolsetName()
      @platform = @context.getPlatformName()

      @toolset_vars = ["CROSS","CC","CFLAGS","PROC_CFLAGS","CXX","CXXFLAGS","PROC_CXXFLAGS","AS","ASFLAGS","PROC_ASFLAGS","LD","LDFLAGS","PROC_LDFLAGS","PROC_LDSCRIPT","AR","ARFLAGS","POSTLD","PROC_POSTLD","PAGE_SIZE","NUCLEUS_VERSION"]
      @tools = ["compile", "compilecxx", "assemble", "cppassemble", "link", "archive", "postlink", "proc_compile", "proc_compilecxx", "proc_assemble", "proc_cppassemble", "proc_link", "proc_postlink"]
      @platform_vars = ["ARCH_CFLAGS","ARCH_CXXFLAGS","ARCH_ASFLAGS","ARCH_LDFLAGS","ARCH_LDSCRIPT","ARCH_PROC_CFLAGS","ARCH_PROC_CXXFLAGS","ARCH_PROC_ASFLAGS","ARCH_PROC_LDFLAGS","ARCH_PROC_LDSCRIPT","ARCH_ARFLAGS"]
      @oflags = {"TARGET_ASFLAGS" => ["ASFLAGS","ARCH_ASFLAGS","INCLUDES"],
                "TARGET_CFLAGS"  => ["CFLAGS", "ARCH_CFLAGS", "INCLUDES"],
                "TARGET_CXXFLAGS" => ["CXXFLAGS","ARCH_CXXFLAGS", "INCLUDES"],
                "TARGET_LDFLAGS"  => ["LDFLAGS","ARCH_LDFLAGS","ARCH_LDSCRIPT", "LD_EXTRA"],
                "TARGET_PROC_ASFLAGS" => ["PROC_ASFLAGS","ARCH_PROC_ASFLAGS","INCLUDES"],
                "TARGET_PROC_CFLAGS"  => ["PROC_CFLAGS", "ARCH_PROC_CFLAGS", "INCLUDES"],
                "TARGET_PROC_CXXFLAGS" => ["PROC_CXXFLAGS","ARCH_PROC_CXXFLAGS", "INCLUDES"],
                "TARGET_PROC_LDFLAGS"  => ["PROC_LDFLAGS","PROC_LDSCRIPT","ARCH_PROC_LDFLAGS","LD_EXTRA"]}

      context.getUniverse().accept(self)
      
      build_root = context.getBuildRoot().getAbsolutePath()
      libs_dir = File.join("$(SYSTEM_HOME)",outputDirectory(),"lib")
      @toolset_vals["LD_EXTRA"] = "-L#{libs_dir}"

      getIncludes()

      # Make module specific flags default to regular flags
      
      if (@toolset_vals["ARCH_PROC_CFLAGS"].empty?)
      	 @toolset_vals["ARCH_PROC_CFLAGS"] = @toolset_vals["ARCH_CFLAGS"]
      end
      if (@toolset_vals["ARCH_PROC_CXXFLAGS"].empty?)
      	 @toolset_vals["ARCH_PROC_CXXFLAGS"] = @toolset_vals["ARCH_CXXFLAGS"]
      end
      if (@toolset_vals["ARCH_PROC_ASFLAGS"].empty?)
      	 @toolset_vals["ARCH_PROC_ASFLAGS"] = @toolset_vals["ARCH_ASFLAGS"]
      end
      if (@toolset_vals["ARCH_PROC_LDFLAGS"].empty?)
      	 @toolset_vals["ARCH_PROC_LDFLAGS"] = @toolset_vals["ARCH_LDFLAGS"]
      end

      generateSettings()
      

      # Write out component configuration file
      
      path = File.join(build_root,outputDirectory(), @conf_filename)
        
      writeHeaderfile(path, @output)
      
      return true
   end
  
   def preVisitPackage(p)
      if isToolsetPackage?(p)
         addVariables(p,@toolset_vars)
         addVariables(p,@tools)
	  elsif isSpecificToolsetPackage?(p) 
	     addVariables(p,@platform_vars)
	  end
	  
      if p.getEnabled() == true and p.getName() == "nu" and p.getParent().getName() == ""
         major = p.getMajorVersionNumber();
         minor = p.getMinorVersionNumber();
         patch = p.getPatchVersionNumber();
         @toolset_vals["NUCLEUS_VERSION"] = (major * 10000 + minor * 100 + patch).to_s
      end
   end

   def addVariables(p,var_list)

      c = p.findComponent(@context.getToolsetName())

      var_list.each { |vars|
         if c.hasProperty(vars)
         	fixed_up = false
            if vars == "ARCH_PROC_LDSCRIPT"
                fixed_up = true
                @toolset_vals["PROC_LDSCRIPT"] = c.getStringProperty(vars)
            elsif vars == "PROC_LDFLAGS"
                fixed_up = true
                @toolset_vals[vars] = "-Wl,-Map=${ProjName}.map "+c.getStringProperty(vars)
            end
            if vars == "ARCH_LDSCRIPT"
                fixed_up = true
                @toolset_vals[vars] = "-T"+c.getStringProperty(vars)
                @toolset_vals["LDSCRIPT"] = c.getStringProperty(vars)
            elsif vars == "LDFLAGS"
                fixed_up = true
                @ldflags = c.getStringProperty(vars)
                @toolset_vals[vars] = "-Wl,-Map=${ProjName}.map "+c.getStringProperty(vars)
            elsif vars == "link"
                fixed_up = true
                @toolset_vals[vars] = c.getStringProperty(vars).gsub(/LDSCRIPT_.\(notdir +.\(1\)\)/, 'LDSCRIPT').gsub(/.\(LDFLAGS\)/, @ldflags)
            end
            if fixed_up == false
                @toolset_vals[vars] = c.getStringProperty(vars)
            end
         else
            @toolset_vals[vars] = ""
         end
      }

      # Get generic process mem management and core component objects
      memmgmt_comp = @context.getUniverse().findFQComponent("nu.os.kern.process.mem_mgmt")
      core_comp = @context.getUniverse().findFQComponent("nu.os.kern.process.core")

      # Check if memory management generic enabled
      if memmgmt_comp.getEnabled() == true
          # Set page size equal to value configured in the core process component
          @toolset_vals["PAGE_SIZE"] = core_comp.getIntegerProperty("page_size").to_s
      else
          # Set page size to 0 when mem management not enabled
          @toolset_vals["PAGE_SIZE"] = "0"
      end
    
   end
   
   def getIncludes
      build_root = @context.getBuildRoot().getAbsolutePath()
      
      # Get system include paths from global property
      system_include_property = @context.getUniverse().getProperty("system_include_paths")
      system_include_relative = system_include_property.split(",")      
      
      # Set toolset SYSTEM_INCLUDES variable
      system_include_relative.each { |p| @abs_system_includes << "-isystem " + "$(SYSTEM_HOME)" + "/" + p.gsub("\\","/") + " "}
      @toolset_vals["SYSTEM_INCLUDES"] = @abs_system_includes
      
      # Set toolset SYSTEM_INCLUDE_PATH variable
      system_include_relative.each { |p| @abs_system_includes_generic << "$(SYSTEM_HOME)" + "/" + p.gsub("\\","/") + " "}      
      @toolset_vals["SYSTEM_INCLUDE_PATH"] = @abs_system_includes_generic
      
      # Get include paths from global property
      include_property = @context.getUniverse().getProperty("include_path")
      include_relative = include_property.split(",")      
      
      # Set toolset INCLUDES variable
      include_relative.each { |p| @abs_includes << "-I" + "$(SYSTEM_HOME)" + "/" + p.gsub("\\","/") + " "}
      @toolset_vals["INCLUDES"] = @abs_includes

      # Set toolset INCLUDE_PATH variable      
      include_relative.each { |p| @abs_includes_generic << "$(SYSTEM_HOME)" + "/" + p.gsub("\\","/") + " "}
      @abs_includes_generic << "$(SYSTEM_HOME)" + " "      
      @toolset_vals["INCLUDE_PATH"] = @abs_includes_generic.chop 
   end

   def generateSettings
      @output += "SYSTEM_HOME = #{@context.getBuildRoot().getAbsolutePath()}\n\n"

      @toolset_vals.each { |name, value|
         @output += name + " = " + value + "\n"
      }

      @oflags.each { |name, vals|
         @output += name + " = "
         vals.each { |val|
            @output += @toolset_vals[val] + " "
         }       
         @output += "\n"    
      }
      @output += "NUCLEUS_LIBS = -lnucleus -lc -lm -lstdc++\n"
      @output += "NUCLEUS_PROC_LIBS = -lnucleus_user -lstdc++ -lgcc\n"
      @output += "TOOLSET = #{@context.getToolsetName()}\n"

      libs_dir = File.join("$(SYSTEM_HOME)",outputDirectory(),"lib")
      @output += "NUCLEUS_LIB = #{libs_dir}/nucleus.lib\n"
   end

   def writeHeaderfile(headerfile_path, lines)
      headerfile = File.new(headerfile_path, "w")
      template = fixupLines(lines)
      template.each { |line| headerfile.write(line) }
      headerfile.close
   end
   
   def fixupLines(lines)
      if Config::CONFIG['host_os'] == 'mswin32'
         lines.gsub!(/\n/,"\r\n")
      end
      lines
   end
 end
 
