require 'fuse-dsl'

FuseType = Java::ComMentorNucleusFuseCoreTypes

class PlatformException < Exception
end

class PlatformDSL < FuseDSL
   attr_reader :comp

   def initialize(filename, universe)
      super(filename)
      @universe = universe
      anoint(@universe, true)
      @parent_dir = File.dirname(@filename)
      @platform = nil
      @dev_ref_count=Hash.new(0)
   end

   def platform(name)
      @platform = @comp = FuseType::Package.new(name, 
         Java::JavaIo::File.new(@parent_dir))
      @universe.addPackage(@platform)
      anoint(@platform, true)
      yield

      # Go through entire list of drivers and the associated ref count
      @dev_ref_count.each { |key, value|

        # Get the driver object
        driver_comp = find_object(@universe, key)

        # Set the reference count for this driver to the initial value (pre configuration settings)
        driver_comp.setIntegerProperty("ref_count", value)
      }
   end

   def include(name)
      anoint_objects_with_name(@universe, name)
   end

   def architecture(name)
      @universe.setProperty("architecture",name)
   end


   def option(opt)
      if opt.is_a? Hash
         name = opt.keys[0]
         value = opt[name]
         if value.is_a? Integer
            value = java::lang::Integer.new(value)
         end
         @universe.setProperty(name, value)
      elsif opt.is_a? String
         super(opt)
      else
         raise PlatformException, "invalid option definition"
      end
   end

   def hardware
      yield
   end

   def device(name)
      # Build a component to represent the device.
      @current_device = @comp = FuseType::Component.new(name, 
         Java::JavaIo::File.new(@parent_dir))
      device_package_name = @platform.getName()
      @current_device.setPackageName(device_package_name)
      @platform.addComponent(@current_device)
      anoint(@current_device, true)
      @obj_stack.push(@current_device)

      # "Parse" out the device settings.
      yield

      @obj_stack.pop
      @current_device = nil
   end

   def driver(name)
      driver_comp = find_object(@universe, name)
      if driver_comp != nil
         anoint_objects_with_name(@universe, name)

         # Set entrypoint of driver instance.
         init_func_name = name.gsub(".", "_") + "_init"
         @current_device.setStringProperty("initfunc", init_func_name)

         # Set an implicit requirement from device to driver.
         requires(name)

         # Increment reference count for each device instance in the .platform file
         @dev_ref_count[name] += 1

         # Add new property to the device instance that links it to this driver
         @current_device.setStringProperty("driver", name)

      else

         # Disable the device referencing this driver since the driver doesn't exist in the model
         @current_device.setEnabled(false)

         # Set an implicit requirement from device to driver.
         # This will prevent a configuration file from over-riding the auto-disable since the
         # requirement will not be met and the build will be aborted
         requires(name)

         # Output warning since the driver associated with this device is not present
         msg = "config warning: The driver " + name + " is not included within the Nucleus tree."
         msg += "  The device utilizing this driver (" + @current_device.getPackageName() + "." 
         msg += @current_device.getName() + ") has been disabled by FUSE."
         puts msg

      end
   end

   def setup_entry(has_setup_entry)
      add_function_property("setup", "setupfunc") if has_setup_entry
   end

   def cleanup_entry(has_cleanup_entry)
      add_function_property("cleanup", "cleanupfunc") if has_cleanup_entry
   end

   def add_function_property(func_name, property_name)
      full_func_name = @current_device.getPackageName + "."
      full_func_name += @current_device.getName
      full_func_name = full_func_name.gsub!(".", "_") + "_#{func_name}"
      @current_device.setStringProperty(property_name, full_func_name)
   end

   def anoint_objects(obj)
      if obj != nil
         anoint(obj, true)
         if obj.is_a? FuseType::Package
            obj.getPackages.each { |package|
               anoint_objects(package)
            }
            obj.getComponents.each { |component|
               anoint_objects(component)
            }
         end
      end
   end

   def anoint_objects_with_name(obj, name)
      if obj != nil
         pieces = name.split(".")
         if pieces.length == 1
            next_obj = obj.findComponent(name)
            if next_obj == nil
               next_obj = obj.findPackage(name)
            end
            anoint_objects(next_obj)
         else
            package = obj.findPackage(pieces[0])
            if package != nil
               anoint(package, true)
               anoint_objects_with_name(package, pieces[1..-1].join("."))
            end
         end
      end
   end

   def find_object(obj, name)
      if obj != nil
         pieces = name.split(".")
         if pieces.length == 1
            next_obj = obj.findComponent(name)
            if next_obj == nil
               next_obj = obj.findPackage(name)
            end
            return next_obj
         else
            package = obj.findPackage(pieces[0])
            if package != nil
               return find_object(package, pieces[1..-1].join("."))
            end
         end
      end
      return nil
   end

   def self.load(filename, universe)
      begin
         Dir.chdir(File.dirname(filename))
         dsl = new(filename, universe)
         dsl.instance_eval(File.read(filename), filename)
         dsl.comp
      rescue Exception => e
         puts "platform error: " + e.message
         nil
      end
   end
end
