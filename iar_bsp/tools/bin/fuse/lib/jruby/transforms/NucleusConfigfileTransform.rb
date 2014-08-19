require 'BuildProperties'

CONFIGFILE_DEFS = <<END
# Generated -- DO NOT EDIT

END

CONFIGFILE_HEADER = <<END
/***********************************************************************
*
*            Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************

************************************************************************
*
*   FILE NAME
*
*       nucleus_gen_cfg.h
*
*   COMPONENT
*
*       Nucleus OS
*
*   DESCRIPTION
*
*       This file contains auto-generated configuration settings for
*       the Nucleus OS.  DO NOT MODIFY.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       None
*
************************************************************************/
END
CONFIGFILE_GEN_HEADER = <<END
/* Generated: %s  */
/* DO NOT MODIFY - This file is auto-generated */
#ifndef NUCLEUS_GEN_CFG_H
#define NUCLEUS_GEN_CFG_H
END

CONFIGFILE_GEN_FOOTER = <<END
#endif /* NUCLEUS_GEN_CFG_H */
END

CONFIGFILE_COMPONENT_HEADER = <<END

/* Component Name: %s */
END

CONFIGFILE_PACKAGE_HEADER = <<END

/* Package Name: %s */
END

class NucleusConfigfileTransform < Transform

   include BuildProperties

   def initialize(phaseList)
      super(phaseList)
      @defines = {}
      @namespace = []
      @outputlines = CONFIGFILE_HEADER
      @outputlines += CONFIGFILE_GEN_HEADER % Time.now
      @config_header_path = nil
      @max_reg_path = 1
      @define_prepend = "CFG_"
      @registry_max_key_length = "CFG_NU_OS_SVCS_REGISTRY_MAX_KEY_LEN"
      @architecture = nil
   end
   
   def executePhase(context)
      @context = context
      @defines.clear

      # Initialize the registry max key length
      @defines[@registry_max_key_length] = @max_reg_path.to_s

      @architecture = context.getUniverse().getProperty("architecture")

      context.getUniverse().getPackages().each {|p|
         p.accept(self)
      }

      # Write out component configuration header file
      build_root = context.getBuildRoot().getAbsolutePath();
      config_header_dir = File.join(build_root, outputDirectory(), "gen", "include")
      if not File.exists?(config_header_dir)
         FileUtils.mkdir_p(config_header_dir)
      end
      
      @config_header_path = File.join(config_header_dir, "nucleus_gen_cfg.h")
         
      if configuration_changed?
         @outputlines += get_def_lines() + CONFIGFILE_GEN_FOOTER
         writeHeaderfile(@config_header_path, @outputlines)
      end
      
      return true
   end

   def preVisitPackage(p)
     @namespace.push(p.getName())

     if isArchitecturePackage?(p)
       if isCurrentArchitecturePackage?(p)
         make_component_define(p)
       end
     else
       make_component_define(p)
       if p.getEnabled() == true and p.getName() == "nu" and p.getParent().getName() == ""
           defineName = make_define_name
           major = p.getMajorVersionNumber();
           minor = p.getMinorVersionNumber();
           patch = p.getPatchVersionNumber();
           fullDefineName = defineName + "major_version"
           @defines[fullDefineName.upcase] = major.to_s
           fullDefineName = defineName + "minor_version"
           @defines[fullDefineName.upcase] = minor.to_s
           fullDefineName = defineName + "patch_version"
           @defines[fullDefineName.upcase] = patch.to_s
       end
     end
   end
  
   def postVisitPackage(p)
     @namespace.pop
   end
   
   def preVisitComponent(c)
     @namespace.push(c.getName())
     
     return if isToolsetComponent?(c) and not isCurrentToolsetComponent?(c)

     return if isArchitectureComponent?(c) and not isCurrentArchitectureComponent?(c)

     # add the "enable" setting for the component to the defines array
     make_component_define(c)

     # only output option/optiongroup defines when component is enabled
     if c.getEnabled()
         # add the component's options to the defines array
         defineName = make_define_name 

         # Check if component has a runlevel
         if c.hasProperty("runlevel")
            set_runlevel_define(c, defineName)
         end

         c.getOptions().each { |o|
            set_option_define(o, c, defineName)
         }
         
         # add the options for each optiongroup
         c.getOptionGroups().each { |g|
            if g.getEnabled() # optiongroups may be disabled
               g.getOptions().each { |o|
                  set_option_define(o, g, defineName + g.getName() + "_")
               }
            end    
         }
     end 
   end
   
   def postVisitComponent(c)
     @namespace.pop()
   end
   
   def isToolsetComponent?(c)
      c.getName() == @context.getToolsetName()
   end
   
   def isCurrentToolsetComponent?(c)
      toolset = @context.getToolsetName()
      platform = @context.getPlatformName()
      abs_name = "#{c.getPackageName()}.#{c.getName()}"

      if "toolsets.#{platform}.#{toolset}" == abs_name
         return true
      elsif "toolsets.#{toolset}" == abs_name
         return true
      elsif "nu.os.arch.#{@architecture}.#{toolset}" == abs_name
         return true
      else
         return false
      end
   end

   def isArchitecturePackage?(p)
      @namespace[0..-2].join(".") == "nu.os.arch"
   end

   def isCurrentArchitecturePackage?(p)
      p.getName() == @architecture
   end

   def isArchitectureComponent?(c)
      c.getArchitecture() != nil
   end

   def isCurrentArchitectureComponent?(c)
      @architecture == c.getArchitecture()
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

   def get_def_lines
     lines = ""

     # output the defines array as a C define all caps
     @defines.each { |name, value|
         # Check to see if the value is #undef - this is just for the enable
         # false case
         if value.to_s == "#undef"
             # Enable false case - #undef the item
             lines += "#undef #{name}\n"
         else
             # All other cases - #define item and value
             lines += "#define #{name}\t#{value}\n"
         end             
     }

     lines
   end

   def make_define_name
      defineName = @define_prepend + @namespace.join("_") + "_"
   end

   def make_component_define(c)
      defineName = make_define_name
      compName = defineName + "enable"
      if c.getEnabled() == true
         # Just generate #define (no value) if enable is true
         @defines[compName.upcase] = ""
      else
         # Generate #undef if enable is false
         @defines[compName.upcase] = "#undef"
      end
   end

   def set_runlevel_define(c, defineName)
      name = (defineName + "runlevel").upcase
      runlevel = c.getIntegerProperty("runlevel")
      @defines[name] = runlevel.to_s
   end

   def set_option_define(o, c, defineName)
      pname = o.getName() + ".enregister"
      name = (defineName + o.getName()).upcase

      if c.hasProperty(pname) and c.getBooleanProperty(pname)

         # See if this is the longest registry path.  This logic removes the defined pre-pend length (ie "CFG_")
         # and adds two characters, one to accommodate null string termination (required for Nucleus code)
         # and the other to accomodate for the starting forward slash '/' in name.
         if (name.length - @define_prepend.length + 2) > @max_reg_path
             
             # Update longest path length (remove length of pre-pended portion and add 2, one for null termination
             # and other to accomadate for the starting forward slash '/' in name.)
             @max_reg_path = name.length - @define_prepend.length + 2
             
             # Update the registry max key length to the new max length
             @defines[@registry_max_key_length] = @max_reg_path.to_s
         end

      else
         value = o.get()
         if value.is_a? String
            @defines[name] = "\"" + o.getString() + "\""
         elsif value.is_a? Integer
            @defines[name] = o.getInteger().to_s
         elsif value.is_a? Float
            @defines[name] = o.getDouble().to_s
         elsif value.is_a? TrueClass or value.is_a? FalseClass
            @defines[name] = o.getBoolean() ? "1" : "0"
         end
      end
   end

   # This method scans the header file generated by the configuration system
   # to determine if changes have been made relative to the
   # current configuration.  NOTE: Changes to registry values do not count
   # as configuration changes.
   def configuration_changed?
      if not File.exists?(@config_header_path)
         return true
      else
         # Parse existing 'nucleus_gen_cfg.h'.
         current_defines = {}
         File.open(@config_header_path, "r") do |config_header_file|
            while (line = config_header_file.gets)
               if line =~ /\s*#define\s*([_a-zA-Z][_a-zA-Z0-9]*)\s*(".*")/
                     current_defines[$1] = $2 != nil ? $2 : "\"\""
               elsif line =~ /\s*#define\s*([_a-zA-Z][_a-zA-Z0-9]*)\s*(\S*)/
                  if $1 != "NUCLEUS_GEN_CFG_H"
                     current_defines[$1] = $2 != nil ? $2 : ""
                  end
               elsif line =~ /\s*#undef\s*([_a-zA-Z][_a-zA-Z0-9]*)\s*/
                  current_defines[$1] = "#undef"
               end
            end
         end

         # Compare with the new version.

         # Make sure number of defines has not changed.         
         if current_defines.length != @defines.length
            return true
         else
            # Compare with the new version.
            current_defines.each do |name, value|
               if not @defines.has_key?(name) or (value != @defines[name])
                  return true
               end
            end
         end

         return false
      end
   end
end
