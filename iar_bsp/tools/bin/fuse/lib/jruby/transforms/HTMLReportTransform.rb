require 'BuildProperties'
require 'fileutils'

class HTMLReportTransform < Transform

   include BuildProperties

   # Set colors for various display elements
   HEADER_COLOR = "#D8D8D8"
   ENABLED_COLOR = "#0B610B"
   DISABLED_COLOR = "#8A0808"
   INFO_COLOR = "#000000"
   REGISTRY_COLOR = "#F2F5A9"
   INFO_BG_COLOR = "#FFFFFF"   
   
   def initialize(phaseList)
      super(phaseList)
      @namespace = []
      @runlevels = {}
      @num_packages = 0
      @num_components = 0
      @objects = []
      @options = {}
      @sources = {}
      @index = []
      @comp_name = nil
      @group_name = nil
   end
   
   def executePhase(context)
      @build_root = context.getBuildRoot()
      @context = context
      context.getUniverse().accept(self)
      writeHTML()
      return true
   end

   def preVisitPackage(p)
      if p.getName() != ""
         @namespace.push(p.getName())
         @num_packages += 1
         updateIndex(p, false)
      end
   end
  
   def postVisitPackage(p)
      if p.getName() != ""
         @namespace.pop
      end
   end
  
   def preVisitComponent(c)
      @comp = c
      @namespace.push(c.getName())
      if c.hasProperty("runlevel") and c.getEnabled()
         if not @runlevels[c.getProperty("runlevel")]
            @runlevels[c.getProperty("runlevel")] = []
         end
         @runlevels[c.getProperty("runlevel")] << getQualifiedName()
      end
      @num_components += 1
      updateIndex(c, true)
      @comp_name = getQualifiedName()
      @objects << [@comp_name, c]
      @sources[@comp_name] = []
      @options[@comp_name] = []
   end
  
   def postVisitComponent(c)
      @comp = nil
      @namespace.pop()
   end
  
   def preVisitOption(o)
      name = o.getName()
      if @comp and @comp.getEnabled()
          obj = @comp
          if obj.hasProperty(name + ".enregister")
              enregister = obj.getBooleanProperty(name + ".enregister")
          else
              grp = @group
              if @group_name and grp.hasProperty(@group_name + ".enregister")
                  enregister = grp.getBooleanProperty(@group_name + ".enregister")
              else
                  enregister = false
              end
          end
      else
          enregister = false
      end
      @namespace.push(name)
      if @group_name
         name = @group_name + "." + name
      end

      if (@comp and @comp.hasProperty(name + ".hidden") and @comp.getBooleanProperty(name + ".hidden")) or
         (@group != nil and @group.hasProperty(@group_name + ".hidden") and @group.getBooleanProperty(@group_name + ".hidden"))
        # Do nothing for now...
      else
        @options[@comp_name] << [name, o, enregister]
      end
   end

   def postVisitOption(o)
      @namespace.pop()
   end
  
   def preVisitOptionGroup(g)
      @group = g
      @namespace.push(g.getName())
      @group_name = g.getName()
   end
  
   def postVisitOptionGroup(g)
      @group = nil
      @namespace.pop()
      @group_name = nil
   end

   def preVisitLibrary(l)
      l.getSourceFiles().each { |filename|
         @sources[@comp_name] << [filename.getAbsolutePath(), 
                                  filename.getName()]
      }
   end

   def getQualifiedName()
      @namespace.join(".")
   end

   def updateIndex(obj, is_component)
      columns = [getQualifiedName(), obj.getEnabled()]
      if obj.getNote()
         columns << obj.getNote().getText()
      else
         columns << ""
      end
      columns << is_component
      @index << columns
   end
 
   def writeHTML()
      abs_output_dir = File.join(@build_root.getAbsolutePath(), 
                                 outputDirectory())

      # Ensure that the output directory exist.
      if not File.exists?(abs_output_dir)
         FileUtils.mkdir_p(abs_output_dir)
      end

      html_filename = File.join(abs_output_dir, "current.imageconfig.html")
 
      File.open(html_filename, "w") do |file|
         file.puts("<html>")
         file.puts("<head>")
         file.puts("<title>Nucleus RTOS Configuration</title>")
         file.puts("</head>")
         file.puts("<body text=#{INFO_COLOR} bgcolor=#{INFO_BG_COLOR}>")

         # Write summary.
         file.puts("<h1>Summary</h1>")
         file.puts("<table border=\"1\">")
         file.puts("<tr><td bgcolor=#{HEADER_COLOR}><b>Platform</b></td>")
         file.puts("<td>#{@context.getPlatformName()}</td></tr>")
         file.puts("<tr><td bgcolor=#{HEADER_COLOR}><b>Toolset</b></td>")
         file.puts("<td>#{@context.getToolsetName()}</td></tr>")
         file.puts("<tr><td bgcolor=#{HEADER_COLOR}><b>Time</b></td><td>#{Time.now}</td></tr>")
         file.puts("<tr><td bgcolor=#{HEADER_COLOR}><b>Number of Components</b></td>")
         file.puts("<td>#{@num_components}</td></tr>")
         file.puts("<tr><td bgcolor=#{HEADER_COLOR}><b>Number of Packages</b></td>")
         file.puts("<td>#{@num_packages}</td></tr>")
         file.puts("</table>")

         # Write run-level table.
         file.puts("<h1>Initialization</h1>")
         file.puts("<table border=\"1\">")
         file.puts("<tr>")
         file.puts("<td bgcolor=#{HEADER_COLOR}><b>Runlevel</b></td><td bgcolor=#{HEADER_COLOR}><b>Name</b></td>")
         file.puts("</tr>")

         @runlevels.keys.sort.each { |level|
            file.puts("<tr>")
            file.puts("<td>#{level}</td><td><br/></td>")
            file.puts("</tr>")
            @runlevels[level].each { |name|
               file.puts("<tr>")
               file.puts("<td><br/></td><td>#{name}</td>")
               file.puts("</tr>")
            }
         }
         file.puts("</table>")

         # Write out objects enable/disable state.
         file.puts("<h1>Package and Component Index</h1>")
         
         # Key for package / component index
         file.puts("<table border=\"0\"><tr>")
         file.puts("<td bgcolor=#{ENABLED_COLOR}><table border=\"1\"><tr><td>&nbsp;&nbsp;&nbsp;</td></tr></table>")
         file.puts("</td><td><b>= Enabled</b></td></tr></table>")
         file.puts("<table border=\"0\"><tr>")
         file.puts("<td bgcolor=#{DISABLED_COLOR}><table border=\"1\"><tr><td>&nbsp;&nbsp;&nbsp;</td></tr></table>")
         file.puts("</td><td><b>= Disabled</b></td></tr></table>")
         
         # Package / component index table
         file.puts("<table border=\"1\">")
         file.puts("<tr><td bgcolor=#{HEADER_COLOR}><b>Name</b></td><td bgcolor=#{HEADER_COLOR}><b>Description</b></td></tr>")
         @index.each { |obj|
            file.puts("<tr>")
            if obj[1]
               color = ENABLED_COLOR
            else
               color = DISABLED_COLOR
            end
            file.puts("<td bgcolor=#{INFO_BG_COLOR}>")
            if obj[3]
               file.puts("<a href=\"##{obj[0]}\">")
               file.puts("<font color=#{color}>#{obj[0]}</font>")
               file.puts("</a>")
            else
               file.puts("<font color=#{color}>#{obj[0]}</font>")
            end
            file.puts("</td>")
            file.puts("<td bgcolor=#{INFO_BG_COLOR}><font color=#{INFO_COLOR}>#{obj[2]}</font></td>")
            file.puts("</tr>")
         }
         file.puts("</table>")

         # Write out component and package info.
         file.puts("<h1>Component Details</h1>")

         # Key for components
         file.puts("<table border=\"0\"><tr>")
         file.puts("<td bgcolor=#{REGISTRY_COLOR}><table border=\"1\"><tr><td>&nbsp;&nbsp;&nbsp;</td></tr></table>")
         file.puts("</td><td><b>= Option in Registry</b></td></tr></table>")         
         
         @objects.each { |object|
            file.puts("<a name=\"#{object[0]}\"></a>")
            file.puts("<h2>#{object[0]}</h2>")

            file.puts("<table border=\"1\"")

            # Write component options.
            file.puts("<tr>")
            file.puts("<td bgcolor=#{HEADER_COLOR}><b>Options</b></td>")

            file.puts("<td>")

            if @options[object[0]].size > 0
               file.puts("<table border=\"1\"")

               file.puts("<tr>")
               file.puts("<td bgcolor=#{HEADER_COLOR}><b>Name</b></td>")
               file.puts("<td bgcolor=#{HEADER_COLOR}><b>Value</b></td>")
               file.puts("<td bgcolor=#{HEADER_COLOR}><b>Description</b></td>")
               file.puts("</tr>")

               @options[object[0]].each { |option|
                  if option[2]
                      color = REGISTRY_COLOR
                  else
                      color = INFO_BG_COLOR
                  end
                  file.puts("<tr>")
                  file.puts("<td bgcolor=#{color}>#{option[0]}</td>")
                  file.puts("<td bgcolor=#{color}>#{option[1].get()}</td>")
                  if option[1].getNote()
                     file.puts(
                        "<td bgcolor=#{color}>#{option[1].getNote().getText()}</td>")
                  else
                     file.puts("<td><b></b></td>")
                  end 
                  file.puts("</tr>")
               }

               file.puts("</table")
            else
               file.puts("<i>No Options</i>")
            end

            file.puts("</td>")
            file.puts("</tr>")

            # Write component files.
            file.puts("<tr>")
            file.puts("<td bgcolor=#{HEADER_COLOR}><b>Files</b></td>")

            file.puts("<td>")

            if @sources[object[0]].size > 0
               file.puts("<table border=\"1\">")

               file.puts("<tr>")
               @sources[object[0]].each { |filename|
                  file.puts("<a type='text/html' href=\"#{filename[0]}\">")
                  file.puts("<font color=#{INFO_COLOR}>#{filename[1]}</font>")
                  file.puts("</a>, ")
               }

               file.puts("</tr>")

               file.puts("</table")
            else
               file.puts("<i>No Files</i>")
            end

            file.puts("</td>")
            file.puts("</tr>")

            file.puts("</table>")
         }

         file.puts("</body>")
         file.puts("</html>")
         file.flush()
      end
   end
end

