class PropertyLoaderTransform < Transform
   def initialize(phaseList)
      super(phaseList)
     
   end
   
   def executePhase(context)
      @context = context
      @reenable_list = []
      new_val = nil
      failed = false

      # NOTE: you must use .keys() here to have the ordering match 
      # the configfile and linkedhashset order
      @context.cliProperties.keys().each { |key|
        original_key = key
        # Strip of internal .*<NUM>* used to work around using a hash
        key = key.sub(/\.\*[0-9]+\*/,'') if key.start_with?('.*')
        # NOTE: We should find a better way to skip "internal" properties.        
        next if key.start_with?(".include")
        
        # Support configuration settings to nu.os_arch => nu.os.arch
        # for backwards compatibility with existing config files
        key = key.sub(/nu\.os_arch\./,'nu.os.arch.')
        
        keyArray = key.split('.')
        if @context.universe.hasProperty(key)
           if @context.universe.getProperty(key) != nil
              new_val = convertValue(@context.universe.getProperty(key).class, 
                                     @context.cliProperties.getProperty(original_key))
              if (keyArray[-1] == 'enable')
                 if new_val == true
                    obj = applyToPath(keyArray[0..-2].join('.'),
                                      :setEnabled, true)
                    setSubTreeEnableProperty(obj, true)
                 else
                    obj = applyToPath(keyArray[0..-2].join('.'),nil, nil)
                    # If the component being disabled is marked
                    # as mandatory, return an error. This is a direct attempt
                    # to disable. In setSubTreeEnableProperty() we check
                    # for indirect disable, but do not error. Indirect is
                    # ignored for backwards compatibility
                    if obj.hasProperty('mandatory')
                       msg = "config error: The component '#{keyArray[0..-2].join('.')}' is mandatory"
                       msg += " and may not be disabled."
                       puts msg
                       failed = true
                    else                       
                       setSubTreeEnableProperty(obj, false)
                    end   
                 end
              else
                 # Flag used to keep track of whether this property should be set or not
                 set_value = true

                 obj = applyToPath(keyArray[0..-2].join('.'),nil, nil)
                 r = obj.getBoundedRange(keyArray[-1])
                 if r != nil
                    if new_val < r.getStart() or new_val > r.getEnd()
                       msg = "config error: The option '#{keyArray[0..-1].join('.')}'"
                       msg += " attempting to be assigned a value of #{new_val.to_s} which is "
                       msg += " outside its bounded range of #{r.getStart()} - #{r.getEnd()}."
                       puts msg
                       failed = true
                       
                       # Set flag showing that this value should not be set
                       set_value = false
                    end
                 else
                    # See if this option has a values list 
                    if obj.hasProperty("#{keyArray[-1]}.values.0")
                    
                       # Get the list values into an array
                       list = obj.getListValues("#{keyArray[-1]}")
                       
                       list_value_found = false
                       if list != nil
                         # Loop through each element of the list and see if the
                         # new value being set is valid
                         list.each { |l| 
                            if l == new_val
                               list_value_found = true
                            end
                         }
                       end
                         
                       # See if the new value was not found in the list
                       if !list_value_found
                          msg = "config error: The option '#{keyArray[0..-1].join('.')}'"
                          msg += " attempting to be assigned a value of #{new_val.to_s} which is outside"
                          msg += " its allowed list of values of ["
                          list.each { |l| msg += "#{l.to_s}, "}
                          msg[-2] = "]"
                          puts msg
                          failed = true
                          
                          # Set flag showing that this value should not be set
                          set_value = false
                       end
                    end
                 end
                 
                 # Set if the value should be set
                 if set_value
                    @context.universe.setProperty(key, new_val)
                 end
              end
           end
        else
           # Assigning attributes in the global namespace that do not exist
           # is permitted.  For example, 'a=12' is OK.  Assigning attributes
           # that are referenced in a package or component is not allowed
           # if the package or component does not exist.

           if keyArray.length == 1
              @context.universe.setProperty(key, 
                 @context.cliProperties.getProperty(key))
           else
              # Attempting to disable a non-existent component/package
              # produces a warning.
              if keyArray[-1] == 'enable' and convertValue(true.class, @context.cliProperties.getProperty(key)) == false
                 msg = "config warning: The component or package '#{keyArray.join('.')}' attempting to be"
                 msg += " disabled does not exist."
                 puts msg
                 failed = false
              # Attempting to enable a non-existent component/package
              # produces an error. Separate test condition used to produce
              # a specific error message.
              elsif keyArray[-1] == 'enable' and convertValue(true.class, @context.cliProperties.getProperty(key)) == true
                 msg = "config error: The component or package '#{keyArray.join('.')}' attempting to be"
                 msg += " enabled does not exist."
                 puts msg
                 failed = true
              # Attempting to set all other non-existent options is an error
              else  
                 msg = "config error: The option '#{keyArray.join('.')}' attempting to be"
                 msg += " assigned does not exist."
                 puts msg
                 failed = true
              end
           end
        end
      }
      if failed
         return false
      end
      
      # re-enable mandatory components that were 
      # indirectly disabled. Attempts to directly
      # disable a mandatory warning generate an error
      # in the setting property loop above
      if @reenable_list.length > 0
         @reenable_list.each { |item| 
            obj = applyToPath([item.getPackageName(),item.getName()].join('.'),
                                      :setEnabled, true)
            setSubTreeEnableProperty(obj, true)
          }
      end
            
      return true
                    # if the co
   end

   private
 
                    # if the co
   def convertValue(type, string_value)
      string_value.strip!
      if type == String
         string_value[0] = "" if string_value.start_with? "\""
         string_value[-1] = "" if string_value.end_with? "\""
         return string_value
      elsif type == Fixnum
         base = 10
         base = 16 if string_value.length > 1 and string_value[0..1] == "0x"
         string_value = "1" if string_value == "true"
         return java::lang::Integer.new(string_value.to_i(base))
      elsif type == Float
         return string_value.to_f
      elsif type == TrueClass or type == FalseClass
         return (string_value == "1" or string_value == "true")
      else
         raise RuntimeError, "unexpected type '#{type}'"
      end
   end
   
   def applyToPath(str, myfunc, val)
      curr_obj = @context.getUniverse()
      next_obj = nil
     
      keyArray = str.split('.')
      i = 0
      while i < keyArray.length do
         searchArray = keyArray[0..i]
         if curr_obj.respond_to?('findPackage')
            next_obj = curr_obj.findPackage(searchArray[-1])
            if next_obj == nil
               next_obj = curr_obj.findComponent(searchArray[-1])
            end
         elsif curr_obj.respond_to?('findOptionGroup')
            next_obj = curr_obj.findOptionGroup(searchArray[-1])
         end

         if next_obj == nil
            break
         else
            if myfunc != nil
               next_obj.send(myfunc, val)
            end
            curr_obj = next_obj
         end
         i = i + 1
      end

      curr_obj
   end
  
   def setSubTreeEnableProperty(obj, val)
      obj.setEnabled(val)
      if obj.respond_to?('findPackage')
         obj.getPackages().each { |p|
            setSubTreeEnableProperty(p, val)
         }      
         obj.getComponents().each { |c|
            setSubTreeEnableProperty(c, val)
         }
      elsif obj.respond_to?('findOptionGroup')
         obj.getOptionGroups().each { |g|
            setSubTreeEnableProperty(g, val)
         }
         if val == false
            # the object is being indirectly disabled
            # track it so we can re-enable it.
            # This requires direct attempts to be
            # handled prior to call setSubTreeEnableProperty
            if obj.hasProperty('mandatory')
               @reenable_list << obj
            end  
         end              
      end
   end
end

