
let currentSS = document.getElementById("ares_current_state")
let wholeSS = document.getElementById("whole_state")
// Set the dimensions and margins of the diagram
var margin = {top: 20, right: 90, bottom: 30, left: 130},
    width = 2800 - margin.left - margin.right,
    height = 1000 - margin.top - margin.bottom;

// append the svg object to the body of the page
// appends a 'group' element to 'svg'
// moves the 'group' element to the top left margin
var svg = d3.select("#svgContainer").append("svg")
    .attr("width", width + margin.right + margin.left)
    .attr("height", height + margin.top + margin.bottom)
  .append("g")
    .attr("transform", "translate("
          + margin.left + "," + margin.top + ")");

var i = 0,
    duration = 750,
    root;

// declares a tree layout and assigns the size
var treemap = d3.tree().size([height, width]);
let init = true;
var xhttp = new XMLHttpRequest();
xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      data = JSON.parse(xhttp.responseText)
      if( init ) newData(data.data);
      else if(data.changed){
        newData(data.data);
      }
    }
};

xhttp.open("GET", "/data", true);
xhttp.send();
setInterval(() => {
  xhttp.open("GET", "/data", true);
  xhttp.send();
}, 4000); 
wholeSSClicked = true;
let newData = function(treeData) {
  // Assigns parent, children, height, depth
  root = d3.hierarchy(treeData, function(d) { return d.children; });
  root.x0 = height / 2;
  root.y0 = 0;   
  if( init ){
    console.log("updating...")
    currentSS.onclick = function(){
      wholeSSClicked =false;
      this.style.color = "#bfbfbf";
      this.style.backgroundColor = "#18191A";
    
      wholeSS.style.color = "#6a6a6a";
      wholeSS.style.backgroundColor = "#e8eEf1";
      console.log(root)
      currentState(root);
      update(root);
    };
    wholeSS.onclick = function(){
      wholeSSClicked = true;
      this.style.color = "#bfbfbf";
      this.style.backgroundColor = "#18191A";
    
      currentSS.style.color = "#6a6a6a";
      currentSS.style.backgroundColor = "#e8eEf1";
      console.log(root)
      wholestate(root);
      update(root);
      prevData = treeData;
    }; 
    wholeSS.click();
    init=false;
  }
  else{
    if( wholeSSClicked )
      wholestate(root);
    else
      currentState(root);
    update(root)
  } 
};

// fetchData()
// setInterval(fetchData,1000);

//Toggling what gets shown
function wholestate(root) {

 let expand = function(d){
    if (!d.children) {
      d.children = d._children;
      d._children = null;
    }
    if(d.children){
      for (let i = 0; i < d.children.length; i++) 
        expand(d.children[i]) 
    }
 };
 expand(root);
}


function currentState(root) {
    let realRoot;
    let non_collapsable = new Set();

    let search = function(node){
      if( node.data.root ){
        realRoot = node;
      }
    };
    // find the real root
    let dfs = function (node,fun) {
      fun(node);
      if( node.hasOwnProperty("children") && node.children ){
        for (let i = 0; i < node.children.length; i++)
          dfs(node.children[i],fun);
      }
    };
    //append every parent on the path to th real root so we don't collapse it
    let buildPath = function(node){
      non_collapsable.add(node)
      if( node.parent) buildPath(node.parent);
    };
    dfs(root,search);
    buildPath(realRoot);

    //Collapse evrything other that the real root's subtree
    dfs(root, function(node){
      if( !non_collapsable.has(node) ) collapse(node);
    });
    wholestate(realRoot)
}


// Collapse the node and all it's children
function collapse(d) {
  if(d.children) {
    d._children = d.children
    d._children.forEach(collapse)
    d.children = null
  }
}

function update(source) {

  // Assigns the x and y position for the nodes
  var treeData = treemap(root);

  // Compute the new tree layout.
  var nodes = treeData.descendants(),
      links = treeData.descendants().slice(1);

  // Normalize for fixed-depth.
  nodes.forEach(function(d){ d.y = d.depth * 180});

  // ****************** Nodes section ***************************

  // Update the nodes...
  var node = svg.selectAll('g.node')
      .data(nodes, function(d) {return d.id || (d.id = ++i); });


  // Enter any new modes at the parent's previous position.
  var nodeEnter = node.enter().insert('g', (d,i,nss) => {
    // if( i!= 0 && d.data.root) return nss[0];
    if( d.data.root  )
      return nss[i].firstChild;  
      return null;
    })
      .attr('class',function(d){
        return d.data.root ? 'node nodeRoot' : 'node nodeNonRoot'
      })
      .attr("transform", function(d) {
        return "translate(" + source.y0 + "," + source.x0 + ")";
    })
    .on('click', click);

  // Add Circle for the nodes
  nodeEnter.append('circle')
      .attr('class',function(d){
        return d.data.root ? 'node nodeRoot' : 'node nodeNonRoot'
      })
      .attr('r', 0)
      .style("fill", function(d) {
          return d.data.root ? "#4682b4" : (d._children ? "#dbedff" : "#fff");
      });

  // Add labels for the nodes
  nodeEnter.append('text')
      .attr('class', d =>{
        return d.data.root ? "rootStateT" : "aresState";
      })
      .attr("dy", ".35em")
      .attr("x", function(d) {
          return d.data.root ? 16 : (d.children || d._children ? -13 : 13);
      })
      .attr("text-anchor", function(d) {
          return d.data.root ? "start" : (d.children || d._children ? "end" : "start");
      })
      .html(function (d) {
        var x = d3.select(this).attr("x");//get the x position of the text
        var y = d3.select(this).attr("dy");//get the y position of the text
        let yn = parseFloat(String(y).replace("em",""))
        let t="";
        let ucts = d.data.ucts.length;
        //Get the ucts
        for (let i = 0; i < ucts; i++) {
          t += "<tspan class='ucts' x="+x+" dy="+(+yn+( (i+1)*.4))+"em>"+ "uct"+ i +" : " + d.data.ucts[i] +"</tspan>"; 
        }
        ucts++;
        let arrS = d.data.state.split("<br/>")
        for (let i = 0; i < arrS.length; i++) {
          t += "<tspan x="+x+" dy="+(+yn+((i+ucts)*(.4)))+"em>"+arrS[i]+"</tspan>";
        }
        return  t;//appending it to the html
      });
      // visited
  nodeEnter.append('text')
          .attr('class', 'aresCount')
          .attr("dy", "0")
          .attr("x", function(d) {
              return 0;
          })
          .attr("text-anchor", function(d) {
              return "middle";
          })
          .text(function(d) { return d.data.visited; });
          
  // UPDATE
  var nodeUpdate = nodeEnter.merge(node);

  // Transition to the proper position for the node
  nodeUpdate.transition()
    .duration(duration)
    .attr("transform", function(d) { 
        return "translate(" + d.y + "," + d.x + ")";
     });

  // Update the node attributes and style
  nodeUpdate.select('circle.node')
    .attr('r', function(d){ 
      return d.data.root ? 19 :12;
    })
    .style("fill", function(d) {
      return d.data.root ? "#4682b4" : (d._children ? "#dbedff" : "#fff");
    })
    .attr('cursor', 'pointer');


  // Remove any exiting nodes
  var nodeExit = node.exit().transition()
      .duration(duration)
      .attr("transform", function(d) {
          return "translate(" + source.y + "," + source.x + ")";
      })
      .remove();

  // On exit reduce the node circles size to 0
  nodeExit.select('circle')
    .attr('r', 1e-6);

  // On exit reduce the opacity of text labels
  nodeExit.select('text')
    .style('fill-opacity', 1e-6);

  // ****************** links section ***************************

  // Update the links...
  var link = svg.selectAll('path.link')
      .data(links, function(d) { return d.id; });

  // Enter any new links at the parent's previous position.
  var linkEnter = link.enter().insert('path', "g")
      .attr("class", "link")
      .attr('d', function(d){
        var o = {x: source.x0, y: source.y0}
        return diagonal(o, o)
      });

  // UPDATE
  var linkUpdate = linkEnter.merge(link);

  // Transition back to the parent element position
  linkUpdate.transition()
      .duration(duration)
      .attr('d', function(d){ return diagonal(d, d.parent) });

  // Remove any exiting links
  var linkExit = link.exit().transition()
      .duration(duration)
      .attr('d', function(d) {
        var o = {x: source.x, y: source.y}
        return diagonal(o, o)
      })
      .remove();

  // Store the old positions for transition.
  nodes.forEach(function(d){
    d.x0 = d.x;
    d.y0 = d.y;
  });

  // Creates a curved (diagonal) path from parent to the child nodes
  function diagonal(s, d) {

    path = `M ${s.y} ${s.x}
            C ${(s.y + d.y) / 2} ${s.x},
              ${(s.y + d.y) / 2} ${d.x},
              ${d.y} ${d.x}`

    return path
  }

  // Toggle children on click.
  function click(d) {
    if (d.children) {
        d._children = d.children;
        d.children = null;
      } else {
        d.children = d._children;
        d._children = null;
      }
    update(d);
  }
}
