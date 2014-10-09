# Load and import a versioned Rapicorn module into the 'Rapicorn' namespace
# Licensed CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0
from Rapicorn1410 import Rapicorn

# Setup the application object, unsing a unique application name.
app = Rapicorn.app_init ("Test Rapicorn Bindings")

# Define the elements of the dialog window to be displayed.
hello_window = """
  <Window id="example-bind-py">
    <Alignment padding="15">
      <VBox spacing="30">
        <HBox>
          <Label markup-text="Editable Text: "/>
          <!-- [EXAMPLE-Label-bind-title] -->
          <TextEditor> <Label markup-text="@bind title"/> </TextEditor>
          <!-- [EXAMPLE-Label-bind-title] -->
        </HBox>
        <HBox homogeneous="true" spacing="15">
          <Button on-click="show" hexpand="1">    <Label markup-text="Show Model"/> </Button>
          <Button on-click="shuffle"> <Label markup-text="Shuffle Model"/> </Button>
        </HBox>
      </VBox>
    </Alignment>
  </Window>
"""

app.load_string (hello_window)

window = app.create_window ("example-bind-py")

# [EXAMPLE-Bindable-ObjectModel]
@Rapicorn.Bindable
class ObjectModel (object):
  def __init__ (self):
    self.title = "Hello There!"
om = ObjectModel()
window.data_context (om.__aida_relay__)
# [EXAMPLE-Bindable-ObjectModel]

def window_command_handler (cmdname, args):
  if cmdname == 'show':
    print 'ObjectModel.title:', om.title
  if cmdname == 'shuffle':
    import random
    v = 'Random bits:' + str (random.randrange (1000000))
    om.title = v
  return True

window.sig_commands += window_command_handler
window.show()
app.loop()
