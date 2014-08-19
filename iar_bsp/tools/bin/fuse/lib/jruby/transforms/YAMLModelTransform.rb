require 'yaml'

class YAMLModelTransform < Transform
   def initialize(phaseList)
      super(phaseList)
      @namespace = []
      @model = {}
      @root_package = {}
      @package_stack = []
      @component_stack = []
      @comp_og_stack = []
      @option
   end
   
   def executePhase(context)
    
      @context = context

      # this visits everything in the universe
      @context.getUniverse().accept(self)
      output = @root_package.to_yaml
      @context.setYamlModel(output)
      #build_path = context.getBuildRoot().getAbsolutePath()
      #File.open(File.join(build_path,'test.yaml'), 'w') do |out|
      #  out.write(output)
      #end
      return true
   end


  def preVisitPackage(p)
    @namespace.push("P(" + p.getName() + ")")
    package = {"name" => "#{p.getName()}",
               "subpackages" => [],
               "components"  => [],
               "properties" => [] }

    setRequiredAttributes(p, package)

    if @package_stack.empty?
       @root_package = package
    else 
       # last item on the stack is the current
       # package's parent. Add current package to
       # the parents subpackages
       parent =  @package_stack.pop()
       parent["subpackages"] << package
       # push the parent, then the current
       # back on the stack. 
       @package_stack.push(parent)
    end
    @package_stack.push(package)
  end
  
  # The required attributes are description and enabled
  # 
  def setRequiredAttributes(composite,hash)
    # Verify we have a note attached
    if composite.getNote() != nil
      hash["description"] = "#{composite.getNote().getText()}" 
    else
      hash["description"] = ""
    end
  
    # Verify we have a display name attached
    if composite.getDisplayName() != nil
      hash["display_name"] = "#{composite.getDisplayName()}" 
    else
      hash["display_name"] = ""
    end
    
    hash["enabled"] = "#{composite.getEnabled()}"    

  end
  
  def addProperty(composite, array, label, traits, description)
    property = {"label" => label,
                "traits" => traits,
                "description" => description
               }
    array << property           
  end
  
  def postVisitPackage(p)
    @namespace.pop
    @package_stack.pop
    post_print_dot_name()
  end
  
  def preVisitComponent(c)
    @namespace.push("C(" + c.getName() + ")")
    print_dot_name()
    # create the component
    component = {"name" => "#{c.getName()}",
                 "options" => [],
                 "groups" => [], 
                 "properties" => [],
                 "reqs" => []}
                 
    setRequiredAttributes(c, component)
 
    # check for a run-level
    if c.hasProperty("runlevel")
       runlevel_list = []
       for i in 0..31
           runlevel_list << "#{i.to_s}"
       end
       traits = {"type" => "integer","value" => "#{c.getProperty("runlevel")}","list" => runlevel_list}
       description = "Run-level at which the given component will be initialized.  "
       description << "Values supported are 0-31 and components at run-level 1 are "
       description << "initialized first, run-level 2 second, etc.  Run-level 0 "
       description << "components are not automatically initialized and require API "
       description << "calls to perform initialization from application context.  "
       description << "NOTE:  Run-levels are assigned to components to ensure proper "
       description << "order-of-initialization.  Changing run-levels can cause system "
       description << "initialization problems and should be done with caution."
       addProperty(c, component["properties"], "runlevel", traits, description)
    end

    c.getRequirements().each { |r|
       component["reqs"] << r.getRequiredComponent()
    }
    
    # set mandatory if required
    if c.hasProperty("mandatory")
      component['mandatory'] = "#{c.getBooleanProperty('mandatory')}"
    end

    # set driver property if present
    if c.hasProperty("driver")
      component['driver'] = "#{c.getStringProperty('driver')}"
    end
    
    # add it to the parent's component list
    parent = @package_stack.pop    
    parent["components"] << component
    @package_stack.push(parent)
    @component_stack.push(component) 
    @comp_og_stack.push(c)
  end
  
  def postVisitComponent(c)
    @namespace.pop()
    @component_stack.pop
    @comp_og_stack.pop
    post_print_dot_name()
  end

  # An option can have either a 
  # component or option group as
  # its parent  
  def preVisitOption(o)
    # Get parent (can be component or option group) from stack and push back on stack
    @parent = @comp_og_stack.pop
    @comp_og_stack.push(@parent)
    
    # Check to see if parent has option with hidden attribute or option group with hidden attribute and it is set to true
    if (@parent.hasProperty(o.getName() + ".hidden") and @parent.getBooleanProperty(o.getName() + ".hidden")) or
       (@parent.hasProperty(@parent.getName() + ".hidden") and @parent.getBooleanProperty(@parent.getName() + ".hidden"))
        # Do nothing now - this option (or group it is in) is hidden
    else
        # Option is not hidden... get YAML info
        @namespace.push("O(" + o.getName() + ")")
        

        option = {"label" => "#{o.getName()}",
                  "traits" => {} }

        object = o.get()
        if object.is_a? String
          option["traits"]["type"] = "string"
          option["traits"]["value"] = "#{o.getString()}"
        elsif object.is_a? Integer
          option["traits"]["type"] = "integer"
          option["traits"]["value"] = "#{o.getInteger()}"
        elsif object.is_a? Float
          option["traits"]["type"] = "double"
          option["traits"]["value"] = "#{o.getDouble()}"
        elsif object.is_a? TrueClass or object.is_a? FalseClass 
          option["traits"]["type"] = "boolean"
          if (o.getBoolean() == true)
            option["traits"]["value"] = "true"
          else
            option["traits"]["value"] = "false"
          end
        elsif object.is_a? Bytes
          option["traits"]["type"] = "guid"

          # we use the stack to get the property from
          # the parent component or option group
          @comp = @comp_og_stack.pop
          @comp_og_stack.push(@comp)
          
          str = @comp.getProperty(o.getName()).to_s
          # Take off '{' and '}', split into a list, then rejoin
          option["traits"]["value"] = "#{str.gsub(/{/,'').gsub(/}/,'').split(',').join(', ')}"
        end 
        
             
        # Verify we have a note attached
        if o.getNote() != nil
          option["description"] = "#{o.getNote().getText()}" 
        else
          option["description"] = ""
        end 

        # Verify we have a display name attached
        if o.getDisplayName() != nil
          option["display_name"] = "#{o.getDisplayName()}" 
        else
          option["display_name"] = ""
        end 
        
        # Get the last component or option group from
        # the stack
        comp_or_og = @comp_og_stack[-1]
        if comp_or_og.hasProperty(o.getName()+".enregister")      
          option["traits"]["enregister"] = "true"
        else  
          option["traits"]["enregister"] = "false"
        end        
                  
        # check if the option has a set of allowed values
        if comp_or_og.hasProperty("#{o.getName()}.values.0")
          list = comp_or_og.getListValues("#{o.getName()}")

          if list != nil
            option["traits"]["list"] = []
            list.each { |l| option["traits"]["list"] << "#{l.to_s}"}
          end

        end  
                  
        # check if the option has a range of allowed values
        if comp_or_og.hasProperty("#{o.getName()}.range")
          r = comp_or_og.getBoundedRange("#{o.getName()}")
          option["traits"]["range"] = ["#{r.getStart()}","#{r.getEnd()}"]
        end  


        parent = @component_stack.pop
        parent["options"] << option
        @component_stack.push(parent)
        print_dot_name()
    end
  end

  def postVisitOption(o)
    @namespace.pop()
    post_print_dot_name()
  end
  
 
  def preVisitOptionGroup(g)
    @comp_og_stack.push(g)    
    print_dot_name()
    if (g.hasProperty(g.getName() + ".hidden") and g.getBooleanProperty(g.getName() + ".hidden"))
        # Do nothing now
    else
        @namespace.push("G("+ g.getName() + ")")
        group = {"name"=> "#{g.getName()}",
                 "properties" => [],      
                 "options" => [] }
        
        setRequiredAttributes(g, group)
        
        parent = @component_stack.pop
        parent["groups"] << group
        @component_stack.push(parent)
        @component_stack.push(group)
    end
  end
  
  # tack the option group off the component
  # stack
  def postVisitOptionGroup(g)
    @namespace.pop()
    @component_stack.pop
    @comp_og_stack.pop 
    post_print_dot_name()   
  end
  
  def print_dot_name
    #puts @namespace.join(".")
  end
  
  def post_print_dot_name
    #puts "Leaving: #{@namespace.join(".")}"
  end  
end
