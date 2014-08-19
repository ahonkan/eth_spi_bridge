require 'BuildProperties'

EDITABLE_SECTION = <<END
/*****************************************************************************/
/*                       USER EDITABLE DEFINES - START                       */
/*****************************************************************************/

%s
/*****************************************************************************/
/*                       USER EDITABLE DEFINES - END                         */
/*****************************************************************************/

END

class RegistryInitTransform < Transform

   include BuildProperties

   def initialize(phaseList)
      super(phaseList)
      @namespace = []
      @comp = nil
      @registry = { "init" => {} }
      @string_lib = Hash.new(0)
      @runlevels = {}
      @init_funcs = []
      @byte_seqs = []
	  @current_group = nil
      (1..31).each { |i|
         @registry["init"][i.to_s] = {}
      }
      @extra_funcs = { "setupfunc" => "setup", "cleanupfunc" => "cleanup" }
      @defs = ""
   end
   
   def executePhase(context)
      @context = context
      context.getUniverse().accept(self)
      build_root = context.getBuildRoot().getAbsolutePath()
      reg_c_file_dir = File.join(build_root, outputDirectory(), "gen", "src")
      if not File.exists?(reg_c_file_dir)
         FileUtils.mkdir_p(reg_c_file_dir)
      end

      reg_c_file_name = File.join(reg_c_file_dir, "reg_impl_mem_node.c")
      reg_file = File.new(reg_c_file_name, "w")
      if @defs != ""
         reg_file.puts(EDITABLE_SECTION % @defs)
      end
      gen_c_registry(reg_file, 'root', @registry)
      reg_file.close
      return true
   end

   def preVisitComponent(c)
      @comp = c
      add_extra_func = lambda { |property_name, reg_name|
         if @comp.hasProperty(property_name) and @comp.getEnabled()
            key = @comp.getPackageName() + "." + @comp.getName()
            key += "." + reg_name
            update_registry(key, "&" + @comp.getStringProperty(property_name))
            @init_funcs << @comp.getStringProperty(property_name)
         end
      }

      if @comp.hasProperty("runlevel") and @comp.getEnabled() and 
              !(@comp.hasProperty("active_artifact") and 
                @comp.getStringProperty("active_artifact") == "process")

         # Get the specified runlevel and update the init table in the 
         # registry with an entry to this components init function.
         runlevel = @comp.getIntegerProperty("runlevel")
         @runlevels[runlevel] ||= 0
         @runlevels[runlevel] += 1

         key = "init." + runlevel.to_s + "." + @runlevels[runlevel].to_s
         value = @comp.getPackageName() + "." + @comp.getName()
         update_registry(key, "/" + value.gsub(".", "/"))

         init_func_name = get_init_func_name(@comp)
         @init_funcs << init_func_name
         update_registry(value + ".entrypoint", "&" + init_func_name)
      end

      @extra_funcs.each { |func_name, reg_name|
         add_extra_func.call(func_name, reg_name)
      }
   end
 
   def postVisitComponent(c)
      @comp = nil
   end
 
   def preVisitOption(o)
      if @comp.getEnabled()   
         pname = o.getName() + ".enregister"
         obj = @comp
         absname = obj.getPackageName() + "." + obj.getName()

         # The option may belong to a group instead of a component.
         if @current_group != nil
            obj = @current_group
	        absname += "." + @current_group.getName()
	     end
 
         if obj.hasProperty(pname) \
               and obj.getBooleanProperty(pname) and obj.getEnabled()
            absname += "." + o.getName()

            if obj.getProperty(o.getName()).is_a? Bytes
               value = (absname + "." + "bytes").gsub(".", "_")
               @byte_seqs << [value, obj.getProperty(o.getName())]
               value = "&" + value + "[0]"
            else
               value = obj.getProperty(o.getName())
            end

            if o.hasProperty(o.getName() + ".binary_build_editable") or 
              (@current_group != nil and @current_group.hasProperty(@current_group.getName() + ".binary_build_editable"))

               @defs += "#define   #{absname.gsub(".", "_").upcase}   "
               if value.is_a? String and value[0] != ?&
                  @defs += "\"" + value + "\""
               elsif value.is_a? TrueClass or value.is_a? FalseClass
                  @defs += (value ? "1" : "0")
               else
                  @defs += "#{value.to_s}"
               end
               @defs += "\r\n"
            end

            update_registry(absname, value)
         end   
      end
   end

   def preVisitOptionGroup(g)
     	@current_group = g
   end
   
   def postVisitOptionGroup(g)
     @current_group = nil
   end
   
   def update_registry(key, value)
      h = @registry
      subkeys = key.split(".")
      prev_h = nil
      subkeys.each { |name|
         if h[name] == nil
            h[name] = {}
         end
         prev_h = h
         h = h[name]
      }
      prev_h[subkeys[-1]] = value
   end

   def gen_c_registry(reg_file, name, h)
      # Write out any needed includes.
      reg_file.puts "#include \"services/reg_impl_mem_node.h\"\n\n"

      # Write out the prototypes for all of the init functions.
      @init_funcs.each { |init_func_name|
         reg_file.puts "extern VOID #{init_func_name} (const CHAR * key, INT);"
      }
      reg_file.puts "\n"

      # Write out each GUID definition.
      @byte_seqs.each { |guid|
         reg_file.puts "const static UNSIGNED_CHAR #{guid[0]}[] = #{guid[1]};"
      }
      reg_file.puts "\n"

      # Remove empty init registries
      @registry["init"].each {|key, value|
         if value.empty?
            @registry["init"].delete(key)
         end
      }

      # Build string library
      gen_string_lib(reg_file, h)

      # Write out string library
      reg_file.puts "static const CHAR reg_str_lib[] ="
      string_lib_content = "\""
      index = 0
      current_pos = 0
      @string_lib.each { |key, value|
        
        # Only process strings in the library
        if @string_lib[key]

            # Add string (with null termination) to library content
            # NOTE: Because some strings are simply a single 1 or 2 digit number, the octal
            #       escape sequence for nul is used to ensure problems don't arise in how
            #       the string content gets created
            string_lib_content <<= key + "\\000"
            
            # Add position of this string to hash for later use
            @string_lib[key] = current_pos
            
            # Update current position in string library content (with null termination)
            current_pos += key.length + 1
            
            index += key.length + 1
            if index >= 80
                # Line break and '\' continuation character after 80 characters
                string_lib_content <<= "\\\n"
                index = 0
            end
        else

            # Put a -1 value for this key for later processing
            @string_lib[key] = -1
            
        end
      }

      # Write out string library with trailing quote and semi-colon
      reg_file.puts string_lib_content + "\";\n\n"

      # Write out all of the "real" nodes.
      gen_c_registry_nodes(reg_file, name, h)

      # Write out the head of the tree.
      reg_file.puts "REG_Memory_Node *root = (REG_Memory_Node *)&reg_root;"
   end

   def gen_string_lib(reg_file, h)
      h.each{ |key, value|
         if value.class == Hash
            gen_string_lib(reg_file, value)

            # Check if this is a string
            if key.is_a? String
                # Save key and set value to show in library
                @string_lib[key] = true
            else
                # Save key and indicate not candidate for library
                @string_lib[key] = false
            end
         else
            # Check if this is a string and longer than minimum allowed
            if key.is_a? String
                # Save key and set value to show in library
                @string_lib[key] = true
            else
                # Save key and indicate not candidate for library
                @string_lib[key] = false
            end
         end
      }
   end

   def gen_c_registry_nodes(reg_file, name, h)
      entries = []
      h.each_with_index { |(key, value), index|
         absname = name + "_" + key
         if value.class == Hash
            gen_c_registry_nodes(reg_file, absname, value)

            # See if this key is in the string library
            if @string_lib[key] != -1
                # Put address of string library and appropriate offset for this string
                key = "(const CHAR *)(reg_str_lib + " + @string_lib[key].to_s + " )"
            else
                key = "\"" + key + "\""
            end
            # Check for last entry in the current table
            if index != h.length - 1
                # Put 0 in unused field when not end of table
                entries <<= "{ " + key + ", { 0 }, &reg_" + absname + "[0] }"
            else
                # Put 0xDEADBEEF in unused field of this entry to indicate end of table
                entries <<= "{ " + key + ", { (UNSIGNED_CHAR *)0xDEADBEEF }, &reg_" + absname + "[0] }"
            end
         else
            # Check for last entry in the current table
            if index != h.length - 1
                entries <<= gen_c_registry_value(name, key, value, false)
            else
                entries <<= gen_c_registry_value(name, key, value, true)
            end
         end
      }

      # Write out the current table of nodes
      reg_file.puts "const static REG_Memory_Node " + "reg_" + name + "[] = {"
      if h.length != 0
         entries.each_with_index { |entry, index|
            if index != entries.length - 1
               reg_file.puts "\t" + entry + ","
            else
               reg_file.puts "\t" + entry
            end
         }
      else
         reg_file.puts "\t{ NU_NULL, { (UNSIGNED_CHAR *)0xDEADBEEF }, (REG_Memory_Node *)0xDEADBEEF }"
      end
      reg_file.puts "};"
   end

   def gen_c_registry_value(name, key, value, lastentry)

      def_name = "#{name[5..-1]}_#{key}".upcase

      # See if this key is in the string library
      if @string_lib[key] != -1
          # Put address of string library and appropriate offset for this string
          key = "(const CHAR *)(reg_str_lib + " + @string_lib[key].to_s + " )"
      else
          key = "\"" + key + "\""
      end
      full_value = "{ " + key + ", { "
      if value.is_a? String and value[0] != ?&
         value = "(UNSIGNED_CHAR *) \"" + value + "\""
      elsif value.is_a? TrueClass or value.is_a? FalseClass
         value = "(UNSIGNED_CHAR *) " + (value ? "1" : "0")
      else
         value = "(UNSIGNED_CHAR *) " + value.to_s
      end

      # Check if this entry was specified as last entry in the table
      if lastentry == true
          # Put 0xDEADBEEF in unused field of this entry to indicate end of table
          lastvalue = " }, (REG_Memory_Node *)0xDEADBEEF }"
      else
          # Put NU_NULL in unused field of this entry since not end of table
          lastvalue = " }, NU_NULL }"
      end

      if @defs.include?(def_name)
         full_value += "(UNSIGNED_CHAR *) " + def_name + lastvalue
      else
         full_value += value + lastvalue
      end
   end

   def get_init_func_name(comp)
      if comp.hasProperty("initfunc")
         comp.getStringProperty("initfunc")
      else
         value = comp.getPackageName() + "." + comp.getName()
         value.gsub(".", "_") + "_init"
      end
   end
end
