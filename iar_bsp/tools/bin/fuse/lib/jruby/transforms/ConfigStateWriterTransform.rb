CONFIGSTATE_HEADER = <<END
##############################################
# CONFIGURATION: 
# GENERATED: %s
##############################################
END

CONFIGSTATE_PACK_HEADER = <<END

##############################################
# Configure Package: %s 
##############################################
END


CONFIGSTATE_COMP_HEADER = <<END

##############################################
# Configure Component:%s 
##############################################
END

class ConfigStateWriterTransform < Transform
   def initialize(phaseList, conf_filename="nucleus_gen.config")
      super(phaseList)
      @settings = {}
      @namespace = []
      @outputlines = CONFIGSTATE_HEADER % Time.now
      @conf_filename = conf_filename
   end
   
   def executePhase(context)
      @context = context

      # limit it to non-toolset packages
      context.getUniverse().getPackages().each {|p|
        if isToolsetPackage?(p) == false
          p.accept(self)
        end
        
      }
      
      # Write out component configuration file
      build_root = context.getBuildRoot().getAbsolutePath();
      path = File.join(build_root,@conf_filename)
        
      writeHeaderfile(path, @outputlines)
      
      return true
  end
  
   def preVisitPackage(p)
     @namespace.push(p.getName())
     @outputlines += CONFIGSTATE_PACK_HEADER % @namespace.join(".")
     make_component_define(p)
     @outputlines += get_def_lines()     
     @settings.clear
   end
  
   def postVisitPackage(p)
     @namespace.pop
   end
   
   def preVisitComponent(c)
     @namespace.push(c.getName())
     @outputlines += CONFIGSTATE_COMP_HEADER % @namespace.join(".")
     
     make_component_define(c)
     
     c.getOptions().each { |o|
       if (c.hasProperty(o.getName() + ".hidden") and c.getBooleanProperty(o.getName() + ".hidden"))
         # Do nothing for now
       else
           value = o.get()
           if value.is_a? String
              @settings[o.getName()] = o.getString()
           elsif value.is_a? Integer
              @settings[o.getName()] = o.getInteger()
           elsif value.is_a? Float
              @settings[o.getName()] = o.getDouble()
           elsif value.is_a? TrueClass or value.is_a? FalseClass 
             if (o.getBoolean() == true)
               @settings[o.getName()] = true
             else
               @settings[o.getName()] = false
             end
           end
        end
     }
     @outputlines += get_def_lines()
     @settings.clear
   end   
   
   def postVisitComponent(c)
     @namespace.pop()
   end
   
   def isToolsetPackage?(p)
      p.getName() == "toolsets"
   end
   
   def isSpecificToolsetPackage?(p)
      (p.getName() == @context.getPlatformName()) and isToolsetPackage?(p.getParent())
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
     @settings.each { |name, value|
         lines += @namespace.join(".") + "." + name + "=" + value.to_s + "\n"
     }

     lines
   end

   def make_component_define(c)
     if c.getEnabled() == true
       @settings["enable"] = 1
     else
       @settings["enable"] = 0
     end    
   end
 end
 
