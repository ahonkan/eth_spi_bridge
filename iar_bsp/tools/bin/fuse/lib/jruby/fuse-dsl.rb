class FuseDSLException < Exception
end

class Bytes
   def initialize(*bytes)
      @bytes = bytes
   end

   def to_s
      me = "{"
      @bytes.each { |byte|
         me += byte.to_s + ","
      }
      me += "}"
   end
end

class GUID < Bytes
   def initialize(*bytes)
      if bytes.length != 16
         raise FuseDSLException, "a GUID definition must contain 12 bytes."
      end
      @bytes = bytes
   end
end

class FuseDSL
   attr_reader :comp

   Component = Java::ComMentorNucleusFuseCoreTypes::Component
   Package = Java::ComMentorNucleusFuseCoreTypes::Package
   Note = Java::ComMentorNucleusFuseCoreTypes::Note
   Option = Java::ComMentorNucleusFuseCoreTypes::Option
   OptionGroup = Java::ComMentorNucleusFuseCoreTypes::OptionGroup
   Library = Java::ComMentorNucleusFuseCoreTypes::Library
   Executable = Java::ComMentorNucleusFuseCoreTypes::Executable
   Process = Java::ComMentorNucleusFuseCoreTypes::Process


   def initialize(filename)
      @filename = filename
      @parent_dir = File.dirname(@filename)
      @component = nil
      @current_option = nil
      @current_artifact = nil
      @current_group = nil
      @current_cflags = []
      @current_cxxflags = []
      @current_asflags = []
      @current_ldflags = []
      @obj_stack = []
   end

   # All of the public methods listed here are a part of the fuse DSL.
   # Any non-public methods (e.g. helper methods) should go at the end
   # of this class in the private block.

   def component(name)
      @comp = Java::ComMentorNucleusFuseCoreTypes::Component.new(name, 
         Java::JavaIo::File.new(@parent_dir))
      anoint(@comp, false)
      @obj_stack.push(@comp)
      yield
      @obj_stack.pop()
   end

   def package(name)
      @comp = Java::ComMentorNucleusFuseCoreTypes::Package.new(name, 
         Java::JavaIo::File.new(@parent_dir))
      anoint(@comp, false)
      @obj_stack.push(@comp)
      yield
      @obj_stack.pop()
   end
   
   def parent(name)
     @comp.setPackageName(name)
   end

   def version(str)
      version_numbers = str.split(".")
      @comp.setMajorVersionNumber(Integer(version_numbers[0]))
      @comp.setMinorVersionNumber(Integer(version_numbers[1]))
      @comp.setPatchVersionNumber(Integer(version_numbers[2]))
   end
   
   def platform(platform_names)
     platform_names.each { |name|
        @comp.addPlatform(name)
     }
   end

   def toolset(name)
      @comp.setToolsetName(name)
   end
   
   def architecture(name)
      @comp.setArchitecture(name)
   end

   def mandatory
      @comp.setBooleanProperty('mandatory', true)
   end

   def active_artifact(artifact)
       @comp.setStringProperty('active_artifact', artifact)
   end
   
   def enable(should_enable)
      @obj_stack[-1].setEnabled(should_enable)
   end

   def description(str)
      @obj_stack[-1].attachNote(
         Java::ComMentorNucleusFuseCoreTypes::Note.new(str))
   end

   def display_name(str)
       @obj_stack[-1].setDisplayName(str)
   end

   def bytes(*bytes)
      Bytes.new(*bytes)
   end

   def guid(*bytes)
      GUID.new(*bytes)
   end

   def default(value)
      # Check that if "values" property was already provided before "default"
      # then the type of "values" matches the one given for "default".

      comp_or_og = @obj_stack[-2]
      if comp_or_og.hasProperty(@current_option.getName() + ".values.0")
         requiredType = comp_or_og.getProperty(@current_option.getName() + ".values.0").class
         if value.class != requiredType
             raise FuseDSLException, "The default value's type does not match the type of values."
         end
      end

      if value.is_a? String
         @current_option.setString(value)
      elsif value.is_a? Integer
         @current_option.setInteger(value)
      elsif value.is_a? Float
         @current_option.setDouble(value)
      elsif value.is_a? TrueClass or value.is_a? FalseClass
         @current_option.setBoolean(value)
      elsif value.is_a? Bytes
         @current_option.set(value)
      end
   end

   def values(object)
      # Check if the "default" property was already set then its type should match
      # with every possible value given in this "values" property.

      requiredType = nil

      # get the next to last item 
      # on the stack, it's either a group or component
      comp_or_og = @obj_stack[-2]

      # Check if "default" property was already set; note it's type
      if comp_or_og.hasProperty(@current_option.getName())
         requiredType = comp_or_og.getProperty(@current_option.getName()).class
      end

      # Verify the types of all elements
      if object.is_a? Array

         object.each_index { |idx|
            if (requiredType == nil)
               requiredType = object[idx].class
               next
            end

            if object[idx].class != requiredType
               raise FuseDSLException, "The values should be of same type and match the default value's type."
            end
         }
         comp_or_og.setListValues("#{@current_option.getName()}", object)

      elsif object.is_a? Range
         @current_option.setBoundedRange(
            Java::ComMentorNucleusFuseUtil::Range.new(object.begin, object.end)
         )
      end
   end

   def enregister(should_enregister)
      if @current_group != nil
        if @current_option != nil
          msg =  "The 'enregister' property is not valid for "
          msg += "'#{@current_option.getName()}' it should be set for the "
          msg += "option group '#{@current_group.getName()}' instead."
          raise FuseDSLException, msg
        else
          #set enregister for the option within the optiongroup
          name = @current_group.getName() + ".enregister"
          @current_group.setBooleanProperty(name, should_enregister)
        end
      else
        name = @current_option.getName() + ".enregister"
        # set enregister for the option within the component
        @comp.setBooleanProperty(name, should_enregister)
      end
   end
   
   def hidden(should_hide)
      if @current_group != nil
        if @current_option != nil
          msg =  "The 'hidden' property is not valid for "
          msg += "'#{@current_option.getName()}' it should be set for the "
          msg += "option group '#{@current_group.getName()}' instead."
          raise FuseDSLException, msg
        else
          #set hidden for the option within the optiongroup
          name = @current_group.getName() + ".hidden"
          @current_group.setBooleanProperty(name, should_hide)
        end
      else
        name = @current_option.getName() + ".hidden"
        # set hidden for the option within the component
        @comp.setBooleanProperty(name, should_hide)
      end
   end

   def binary_build_editable
      if @current_option != nil
         if (@current_group != nil and has_property(@current_group, @current_group.getName + ".enregister")) or 
             has_property(@comp, @current_option.getName + ".enregister")
            # Set the option binary_build_editable
            @current_option.setBooleanProperty(@current_option.getName + ".binary_build_editable", true)
         else
            msg  = "The 'binary_build_editable' property is not valid for #{@current_option.getName}, "
            msg += "it is only valid for enregistered groups or options"
            raise FuseDSLException, msg
         end
      elsif @current_group != nil
         if has_property(@current_group, @current_group.getName + ".enregister")
            # Set the group binary_build_editable
            @current_group.setBooleanProperty(@current_group.getName + ".binary_build_editable", true)
         else
            msg  = "The 'binary_build_editable' property is not valid for #{@current_group.getName}, "
            msg += "it is only valid for enregistered groups or options"
            raise FuseDSLException, msg
         end
      else
         msg =  "The 'binary_build_editable' property is only valid for options and groups"
         raise FuseDSLException, msg
      end
   end

   def requires(name)
      @comp.addRequirement(
         Java::ComMentorNucleusFuseCoreTypes::Requirement.new(
            @comp.getPackageName() + "." + @comp.getName(), name))
   end
   
   def option(name)
      if not name =~ /^[_A-Za-z][\w]+$/
         msg =  "The option named '#{name}' has an illegal format. Option"
         msg += " names can only contain letters, numbers, and underscores."
         raise FuseDSLException, msg
      end

      @current_option = 
         Java::ComMentorNucleusFuseCoreTypes::Option.new(name,  @obj_stack[-1])
      @obj_stack.push(@current_option)

      yield

      # @obj_stack[-2] is either a component or optiongroup
      @obj_stack[-2].addOption(@current_option)

      if @current_group != nil 
        name = @current_option.getName() + ".enregister"
        # set enregister for the options within the group, the syntax has 
        # all options within an optiongroup sharing the same enregister 
        # property value
        value = @current_group.getBooleanProperty(
           @current_group.getName() + ".enregister" )
        @current_group.setBooleanProperty(name, value)
      end

      @current_option = nil
      @obj_stack.pop()
   end

   def group(name)
     @current_group = 
         Java::ComMentorNucleusFuseCoreTypes::OptionGroup.new(name)
     # set the default group enregister property to false         
     @current_group.setBooleanProperty(name + ".enregister", false)
     @obj_stack.push(@current_group)
     yield
     @comp.addOptionGroup(@current_group)
     @current_group = nil
     @obj_stack.pop()
   end
   
   def library(name)
      @current_artifact = 
         Java::ComMentorNucleusFuseCoreTypes::Library.new(name)
      yield
      @comp.addLibrary(@current_artifact)
   end
   
   def executable(name)
      @current_artifact = 
         Java::ComMentorNucleusFuseCoreTypes::Executable.new(name)
      yield
      @comp.addExecutable(@current_artifact)
      update_flag_properties(@current_ldflags, "ldflags", name)
      @current_ldflags = []
   end

   def process(name)
      @current_artifact = 
          Java::ComMentorNucleusFuseCoreTypes::Process.new(name)
      yield
      @comp.addProcess(@current_artifact)
      update_flag_properties(@current_ldflags, "ldflags", name)
      @current_ldflags = []
   end
   
   def libraries
      libs = yield
      libs.each { |l|
         @current_artifact.addLibrary(Java::JavaIo::File.new(
            Java::JavaIo::File.new(@parent_dir), l))
      }
   end

   def cflags(tool_flags)
      handle_flags('cflags', tool_flags, @current_cflags)
   end

   def cxxflags(tool_flags)
      handle_flags('cxxflags', tool_flags, @current_cxxflags)
   end

   def asflags(tool_flags)
      handle_flags('asflags', tool_flags, @current_asflags)
   end

   def ldflags(tool_flags)
      handle_flags('ldflags', tool_flags, @current_ldflags)
   end

   def sources
      srcs = yield
      srcs.each { |s|
         file = Java::JavaIo::File.new(Java::JavaIo::File.new(@parent_dir), s)
         @current_artifact.addSourceFile(file)
         
         update_flag_properties(@current_cflags, "cflags", file.getName())
         update_flag_properties(@current_cxxflags, "cxxflags", file.getName())
         update_flag_properties(@current_asflags, "asflags", file.getName())
      }
      @current_cflags = []
      @current_cxxflags = []
      @current_asflags = []
   end

   def runlevel(level)
      @comp.setIntegerProperty("runlevel", level)
   end

   def includepath(paths)
      assert_class_in([Component], @obj_stack[-1].class, 'includepath')

      if paths.is_a? String
         @comp.setStringProperty('includepath', paths)
      elsif paths.is_a? Array
         @comp.setStringProperty('includepath', paths.join(':'))
      end
   end
    
   def systemincludepath(paths)
      assert_class_in([Component], @obj_stack[-1].class, 'systemincludepath')
      if paths.is_a? String
         @comp.setStringProperty('systemincludepath', paths)
      elsif paths.is_a? Array
         @comp.setStringProperty('systemincludepath', paths.join(':'))
      end      
   end   

   def ldscript(script_map)
      assert_class_in([Executable], @current_artifact.class, 'ldscript')

      if script_map.is_a? Hash
         toolset_name = script_map.keys[0]
         script_path =  File.join(@parent_dir, script_map[toolset_name])
         property_name = "#{toolset_name}.ldscript"
         @current_artifact.setStringProperty(property_name, script_path)
      else
         msg = "the '#{name}' attribute was passed argument '#{script_map}'."
         raise FuseDSLException, msg
      end
   end

   def self.load(filename)
      begin
         Dir.chdir(File.dirname(filename))
         dsl = new(filename)
         dsl.instance_eval(File.read(filename), filename)
         dsl.comp
      rescue FuseDSLException => e
         puts "metadata error: '#{filename}': " + e.message
         nil
      rescue SyntaxError => e
         puts "syntax error: " + e.message
         nil
      rescue Exception => e
         puts "error: " + e.message
         nil
      end
   end

private

   def anoint(obj, value)
      obj.setBooleanProperty("anointed", value)
   end

   def assert_class_in(expected_classes, parent_class, keyword)
      keyword_map = {
          Component   => "component",
          Package     => "package",
          Note        => "note",
          Option      => "option",
          OptionGroup => "group",
          Library     => "library",
          Executable  => "executable"
      }

      if expected_classes.select { |item| item == parent_class }.size == 0
         msg = "The keyword '#{keyword}' is not allowed in"
         msg += " '#{keyword_map[parent_class]}' block context."
         raise FuseDSLException, msg
      end
   end

   def handle_flags(name, tool_flags, flag_list)
      if tool_flags.is_a? Hash
         name = tool_flags.keys[0]
         value = tool_flags[name]
         flag_list << [name, value]
      else
         msg = "the '#{name}' attribute was passed argument '#{tool_flags}'."
         raise FuseDSLException, msg
      end
   end

   def update_flag_properties(flags, flag_type, filename)
      if flags.length > 0
         flags.each { |tool_flags|
            name = tool_flags[0] + "." + flag_type + "." + filename
            value = tool_flags[1]
            @current_artifact.setStringProperty(name, value)
         }
      end
   end

   def has_property(comp, property)
      comp.hasProperty(property) and comp.getBooleanProperty(property) == true
   end

end

